#pragma once

/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel, and others
 *****************************************************************************/

#include "phasar/Config/phasar-config.h"
#include "phasar/Utils/ChronoUtils.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/TemplateString.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/WithColor.h"

#include <chrono> // high_resolution_clock::time_point, milliseconds
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <source_location>
#include <string> // string
#include <type_traits>
#include <utility>

#ifdef PHASAR_THREAD_SAFE_PAMM
#include <atomic>
#include <mutex>
#else
#include "phasar/Utils/Average.h"
#endif

namespace llvm {
class raw_ostream;
} // namespace llvm

/// \file
/// PhASAR's performance measurement mechanism (PAMM).
/// Provides counters, histograms, and timers for fine-grained performance
/// tracking. Use the macros from PAMMMacros.h to declare static instances
/// of these metrics. On construction, they are automatically registered in the
/// static pamm::Registry, so that report-generation can be automated.
/// The macros expect a valid C++ identifier as name, and a severify-level
/// (Core/Full) under which the metric should be active.
///
/// Metrics are categorizable via a pamm::Category. The
/// performance report groups by category; categories can be enabled/disabled at
/// runtime. By convention, the macros register the metrics under the category
/// 'PAMMCategory'; create a new category with PAMM_CATEGORY(name).
///
/// Currently, PAMM can be run with all metrics (severity level 2 = Full) or
/// only core metrics (severity level 1 = Core) enabled. The severity level can
/// be changed when building PhASAR. The CMake option is
/// -DPHASAR_ENABLE_PAMM=[Off/Core/Full].
///
/// Thread-safety:
/// If PhASAR is configured with the cmake-option -DPHASAR_THREAD_SAFE_PAMM=ON,
/// PAMM provides some thread-safety guarentees. Generally, construction and
/// destruction of metrics is NOT thread-safe. The pamm::Registry is therefore
/// also NOT thread-safe. Adding data, i.e, incrementing counters, adding to
/// histograms, starting/stopping timers is protected by suitable
/// synchronization primitives and can therefore be safely performed
/// concurrently.

namespace psr {

/// Defines the different level of severity of PAMM's performance evaluation
enum class PAMM_SEVERITY_LEVEL { Off = 0, Core, Full }; // NOLINT

// NOLINTNEXTLINE
inline constexpr PAMM_SEVERITY_LEVEL PAMM_CURR_SEV_LEVEL =
#if defined(PAMM_FULL)
    PAMM_SEVERITY_LEVEL::Full;
#elif defined(PAMM_CORE)
    PAMM_SEVERITY_LEVEL::Core;
#else
    PAMM_SEVERITY_LEVEL::Off;
#endif

namespace pamm {

class Registry;

template <bool Enabled = PAMM_CURR_SEV_LEVEL != PAMM_SEVERITY_LEVEL::Off>
class Category {
public:
  explicit constexpr Category(llvm::StringLiteral Name,
                              bool IsEnabled = true) noexcept
      : Name(Name), IsEnabled(IsEnabled) {}

  constexpr operator llvm::StringRef() const noexcept { return Name; }

  [[nodiscard]] constexpr llvm::StringRef name() const noexcept { return Name; }

  [[nodiscard]] constexpr bool isEnabled() const noexcept {
    return IsEnabled.load(std::memory_order_relaxed);
  }

  constexpr void disable() noexcept { IsEnabled = false; }
  constexpr void enable() noexcept { IsEnabled = true; }

private:
  llvm::StringRef Name;
#ifdef PHASAR_THREAD_SAFE_PAMM
  std::atomic_bool
#else
  bool
#endif
      IsEnabled = true;
};

template <> class Category<false> {
public:
  explicit constexpr Category(llvm::StringLiteral Name,
                              bool /*IsEnabled*/ = true) noexcept
      : Name(Name) {}

  constexpr operator llvm::StringRef() const noexcept { return Name; }

  [[nodiscard]] constexpr llvm::StringRef name() const noexcept { return Name; }

  [[nodiscard]] constexpr auto isEnabled() const noexcept {
    return std::false_type{};
  }

  constexpr void disable() noexcept {}

  void enable() noexcept {
    llvm::WithColor::warning()
        << "Cannot enable PAMM category '" << Name
        << "', because PAMM is disabled at compile time\n";
  }

private:
  llvm::StringRef Name;
};

namespace detail {
struct PAMMBase {
  std::source_location Loc{};
};
#ifdef PHASAR_THREAD_SAFE_PAMM
struct CounterBase : PAMMBase {
  std::atomic_ptrdiff_t Ctr{};

  constexpr void add(ptrdiff_t Offset) noexcept {
    Ctr.fetch_add(Offset, std::memory_order_relaxed);
  }

  [[nodiscard]] constexpr ptrdiff_t value() const noexcept {
    return Ctr.load(std::memory_order_relaxed);
  }
};

struct MinMaxCounterBase : PAMMBase {
  std::atomic_size_t Min = SIZE_MAX;
  std::atomic_size_t Max = 0;
  std::atomic_size_t Sum{};
  std::atomic_size_t NumSamples{};

  constexpr void add(size_t Offset) noexcept {
    Sum.fetch_add(Offset, std::memory_order_relaxed);
    NumSamples.fetch_add(1, std::memory_order_relaxed);

    {
      auto PrevMin = Min.load(std::memory_order_relaxed);
      while (PrevMin > Offset &&
             Min.compare_exchange_weak(PrevMin, Offset,
                                       std::memory_order_relaxed)) {
      }
    }
    {
      auto PrevMax = Max.load(std::memory_order_relaxed);
      while (PrevMax < Offset &&
             Max.compare_exchange_weak(PrevMax, Offset,
                                       std::memory_order_relaxed)) {
      }
    }
  };

  [[nodiscard]] double getAverage() noexcept {
    return double(Sum.load(std::memory_order_relaxed)) /
           double(getNumSamples());
  }
  [[nodiscard]] size_t getNumSamples() noexcept {
    return NumSamples.load(std::memory_order_relaxed);
  }

  void clear() noexcept {
    Min = SIZE_MAX;
    Max = 0;
    Sum = 0;
    NumSamples = 0;
  }
};

struct TimerBase : PAMMBase {

  static constexpr std::chrono::steady_clock::time_point None =
      std::chrono::steady_clock::time_point::max();

  std::atomic<std::chrono::steady_clock::time_point> StartPoint = None;
  // no atomic arithmetic with nanoseconds, so use int64_t here:
  std::atomic<int64_t> Acc{};

  [[nodiscard]] constexpr bool isStarted() const noexcept {
    // XXX: Revisit the memory-order here: Can it be relaxed?
    return StartPoint.load(std::memory_order_acquire) != None;
  }

  void start() noexcept {
    assert(!isStarted() && "Starting an already running timer is not allowed");
    StartPoint.store(std::chrono::steady_clock::now(),
                     std::memory_order_release);
  }

  void stop() noexcept {
    auto EndPoint = std::chrono::steady_clock::now();
    auto StartPoint =
        this->StartPoint.exchange(None, std::memory_order_acq_rel);
    assert(StartPoint != None && "Stopping a not-running timer is not allowed");
    Acc.fetch_add((EndPoint - StartPoint).count(), std::memory_order_relaxed);
  }

  void reset() noexcept {
    Acc = 0;
    StartPoint = None;
  }

  [[nodiscard]] constexpr auto elapsedNanos() const noexcept {
    return std::chrono::nanoseconds(Acc.load(std::memory_order_relaxed));
  }

  [[nodiscard]] constexpr std::chrono::nanoseconds
  pendingNanos() const noexcept {
    auto StartPoint = this->StartPoint.load(std::memory_order_relaxed);
    if (StartPoint == None) {
      return {};
    }
    auto EndPoint = std::chrono::steady_clock::now();
    return EndPoint - StartPoint;
  }

  [[nodiscard]] hms elapsed() const noexcept { return elapsedNanos(); }
};

#else
struct CounterBase : PAMMBase {
  ptrdiff_t Ctr{};

  constexpr void add(ptrdiff_t Offset) noexcept { Ctr += Offset; }

  [[nodiscard]] constexpr ptrdiff_t value() const noexcept { return Ctr; }
};

struct MinMaxCounterBase : PAMMBase {
  size_t Min = SIZE_MAX;
  size_t Max = 0;
  Sampler Avg{};

  constexpr void add(size_t Offset) noexcept {
    Avg.addSample(Offset);
    if (Offset > Max) {
      Max = Offset;
    }
    if (Offset < Min) {
      Min = Offset;
    }
  };

  [[nodiscard]] double getAverage() noexcept { return Avg.getAverage(); }
  [[nodiscard]] size_t getNumSamples() noexcept { return Avg.getNumSamples(); }

  void clear() noexcept {
    Min = SIZE_MAX;
    Max = 0;
    Avg = {};
  }
};

struct TimerBase : PAMMBase {

  static constexpr std::chrono::steady_clock::time_point None =
      std::chrono::steady_clock::time_point::max();

  std::chrono::steady_clock::time_point StartPoint = None;
  std::chrono::nanoseconds Acc{};

  [[nodiscard]] constexpr bool isStarted() const noexcept {
    return StartPoint != None;
  }

  void start() noexcept {
    assert(!isStarted() && "Starting an already running timer is not allowed");
    StartPoint = std::chrono::steady_clock::now();
  }

  void stop() noexcept {
    assert(isStarted() && "Stopping a not-running timer is not allowed");
    auto EndPoint = std::chrono::steady_clock::now();
    Acc += (EndPoint - StartPoint);
    StartPoint = None;
  }

  void reset() noexcept {
    Acc = {};
    StartPoint = None;
  }

  [[nodiscard]] constexpr std::chrono::nanoseconds
  elapsedNanos() const noexcept {
    return Acc;
  }

  [[nodiscard]] constexpr std::chrono::nanoseconds
  pendingNanos() const noexcept {
    if (!isStarted()) {
      return {};
    }
    auto EndPoint = std::chrono::steady_clock::now();
    return EndPoints - StartPoint;
  }

  [[nodiscard]] hms elapsed() const noexcept { return Acc; }
};

#endif

struct HistogramBase : PAMMBase {
  llvm::StringMap<uint64_t> HistData{};
#ifdef PHASAR_THREAD_SAFE_PAMM
  std::mutex Mx{};
#endif
};

template <TemplateString Name, const auto *Cat> struct Qualified {
  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }
};
} // namespace detail

class Registry;

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Counter : private detail::CounterBase,
                private detail::Qualified<Name, Cat> {
  friend Registry;

public:
  using detail::Qualified<Name, Cat>::qualifier;

  inline explicit Counter(
      std::source_location Loc = std::source_location::current()) noexcept;

  using detail::CounterBase::add;
  using detail::CounterBase::value;

  constexpr void operator++() noexcept { add(1); }
  constexpr void operator++(int) noexcept { add(1); }
  constexpr void operator+=(ptrdiff_t Offset) noexcept { add(Offset); }
  constexpr void operator-=(ptrdiff_t Offset) noexcept { add(-Offset); }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Counter &C) {
    return OS << Cat->name() << "::" << Name << ": " << C.Ctr;
  }

private:
  [[nodiscard]] constexpr detail::CounterBase *base() noexcept { return this; }
};

template <TemplateString Name, const Category<false> *Cat>
class Counter<false, Name, Cat> : private detail::Qualified<Name, Cat> {
public:
  using detail::Qualified<Name, Cat>::qualifier;

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void add(ptrdiff_t Offset) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++() noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++(int) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  operator+=(ptrdiff_t Offset) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  operator-=(ptrdiff_t Offset) noexcept {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Counter /*C*/) {
    return OS << Cat->name() << "::" << Name;
  }

  [[nodiscard]] constexpr std::nullopt_t value() const noexcept {
    return std::nullopt;
  }
};

namespace detail {
struct IsCounterImpl {
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  static std::true_type test(const Counter<Enabled, Name, Cat> &);

  static std::false_type test(...);
};

template <typename T>
concept IsCounter =
    decltype(IsCounterImpl::test(std::declval<const T &>()))::value;
} // namespace detail

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class MinMaxCounter : private detail::MinMaxCounterBase,
                      private detail::Qualified<Name, Cat> {
  friend Registry;

public:
  using detail::Qualified<Name, Cat>::qualifier;

  inline explicit MinMaxCounter(
      std::source_location Loc = std::source_location::current()) noexcept;

  using detail::MinMaxCounterBase::add;

  constexpr void operator++() noexcept { add(1); }
  constexpr void operator++(int) noexcept { add(1); }
  constexpr void operator+=(size_t Offset) noexcept { add(Offset); }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const MinMaxCounter &C) {
    return OS << Cat->name() << "::" << Name << ": min(" << C.Min << "), max("
              << C.Max << "), avg: " << llvm::format("%g", C.getAverage())
              << ", #samples(" << C.getNumSamples() << ')';
  }

private:
  [[nodiscard]] constexpr detail::CounterBase *base() noexcept { return this; }
};

template <TemplateString Name, const Category<false> *Cat>
class MinMaxCounter<false, Name, Cat> : private detail::Qualified<Name, Cat> {
public:
  using detail::Qualified<Name, Cat>::qualifier;

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void add(size_t Offset) noexcept {}

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++() noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++(int) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  operator+=(size_t Offset) noexcept {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       MinMaxCounter /*C*/) {
    return OS << Cat->name() << "::" << Name;
  }
};

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Histogram : private detail::HistogramBase,
                  private detail::Qualified<Name, Cat> {
  friend Registry;

public:
  using detail::Qualified<Name, Cat>::qualifier;

  inline explicit Histogram(
      std::source_location Loc = std::source_location::current()) noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Histogram &H) {
    OS << Cat->name() << "::" << Name << ":\n";
    OS << "Value\t| #Occurrences\n";
#ifdef PHASAR_THREAD_SAFE_PAMM
    std::lock_guard Lck(H.Mx);
#endif
    for (const auto &[Dat, Val] : H.HistData) {
      OS << Dat << "\t| " << Val << '\n';
    }
    return OS;
  }

  void add(llvm::StringRef DataPointId, uint64_t Increment) {
    if (Cat->isEnabled()) {
#ifdef PHASAR_THREAD_SAFE_PAMM
      std::lock_guard Lck(Mx);
#endif
      this->HistData[DataPointId] += Increment;
    }
  }
  template <has_adl_to_string_v T>
    requires(!std::convertible_to<T, llvm::StringRef>)
  void add(T &&DataPointId, uint64_t Increment) {
    add(psr::adl_to_string(PSR_FWD(DataPointId)), Increment);
  }

  // For unit-tests
  [[nodiscard]] const auto &internalRawData() const noexcept {
    return HistData;
  }

private:
  [[nodiscard]] constexpr detail::HistogramBase *base() noexcept {
    return this;
  }
};

template <TemplateString Name, const Category<false> *Cat>
class Histogram<false, Name, Cat> : private detail::Qualified<Name, Cat> {
  friend Registry;

public:
  using detail::Qualified<Name, Cat>::qualifier;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Histogram /*H*/) {
    return OS << Cat->name() << "::" << Name << "\n";
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  add(llvm::StringRef /*DataPointId*/, uint64_t /*Increment*/) {}

  template <has_adl_to_string_v T>
    requires(!std::convertible_to<T, llvm::StringRef>)
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void add(T && /*DataPointId*/,
                                                  uint64_t /*Increment*/) {}
};

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Timer : private detail::TimerBase, private detail::Qualified<Name, Cat> {
  friend Registry;
  template <bool Enabled2> friend class ScopedTimer;

public:
  using detail::Qualified<Name, Cat>::qualifier;

  inline explicit Timer(
      std::source_location Loc = std::source_location::current()) noexcept;

  using detail::TimerBase::elapsed;
  using detail::TimerBase::elapsedNanos;
  using detail::TimerBase::reset;
  using detail::TimerBase::start;
  using detail::TimerBase::stop;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Timer &T) {
    return OS << Cat->name() << "::" << Name << ": " << T.elapsed();
  }

private:
  [[nodiscard]] constexpr detail::TimerBase *base() noexcept { return this; }
};

template <TemplateString Name, const Category<false> *Cat>
class Timer<false, Name, Cat> : private detail::Qualified<Name, Cat> {
public:
  using detail::Qualified<Name, Cat>::qualifier;

  LLVM_ATTRIBUTE_ALWAYS_INLINE void start() noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE void stop() noexcept {}

  [[nodiscard]] constexpr std::chrono::nanoseconds
  elapsedNanos() const noexcept {
    return {};
  }

  [[nodiscard]] hms elapsed() const noexcept { return {}; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Timer /*T*/) {
    return OS << Cat->name() << "::" << Name;
  }
};

template <bool Enabled> class ScopedTimer {
public:
  template <TemplateString Name, const Category<Enabled> *Cat>
  constexpr ScopedTimer(Timer<Enabled, Name, Cat> &Tm) : Tm(&Tm) {
    if (Cat->isEnabled()) {
      Tm.start();
    }
  }

  ~ScopedTimer() {
    if (Tm->isStarted()) {
      Tm->stop();
    }
  }

  ScopedTimer(const ScopedTimer &) = delete;
  ScopedTimer &operator=(const ScopedTimer &) = delete;
  ScopedTimer(ScopedTimer &&) = delete;
  ScopedTimer &operator=(ScopedTimer &&) = delete;

private:
  NonNullPtr<detail::TimerBase> Tm;
};

template <> class ScopedTimer<false> {
public:
  template <TemplateString Name, const Category<false> *Cat>
  constexpr ScopedTimer(Timer<false, Name, Cat> &Tm) {}
};

class Registry {
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  friend class Counter;
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  friend class MinMaxCounter;
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  friend class Histogram;
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  friend class Timer;

public:
  static Registry &instance() {
    static Registry Reg;
    return Reg;
  }

  void printCounters(llvm::raw_ostream &OS) const;
  void printCounters(llvm::raw_ostream &OS, const Category<true> &Cat) const;
  void printCounters(llvm::raw_ostream &OS, const Category<false> &Cat) const {}

  void printMinMaxCounters(llvm::raw_ostream &OS) const;
  void printMinMaxCounters(llvm::raw_ostream &OS,
                           const Category<true> &Cat) const;
  void printMinMaxCounters(llvm::raw_ostream &OS,
                           const Category<false> &Cat) const {}

  void printHistograms(llvm::raw_ostream &OS) const;
  void printHistograms(llvm::raw_ostream &OS, const Category<true> &Cat) const;
  void printHistograms(llvm::raw_ostream &OS,
                       const Category<false> &Cat) const {}

  void printTimers(llvm::raw_ostream &OS) const;
  void printTimers(llvm::raw_ostream &OS, const Category<true> &Cat) const;
  void printTimers(llvm::raw_ostream &OS, const Category<false> &Cat) const {}

  [[nodiscard]] const Category<true> *findCategory(llvm::StringRef Name) const;

  // Sets all counters, histograms and timers back to 0. Useful for unit-tests
  void reset() noexcept;

  // Erases all registered PAMM-elements. Useful for unit-tests
  void clear() noexcept;

private:
  void registerImpl(auto *Elem, auto &Into, const Category<true> *Cat,
                    llvm::StringRef Name, llvm::StringRef ElemKind,
                    std::source_location Loc) {
    assert(Elem != nullptr);
    assert(Cat != nullptr);

    RegisteredCategories.try_emplace(Cat->name(), Cat);

    auto [It, Inserted] =
        Into[Cat].try_emplace(llvm::StringRef(Name), Elem->base());
    if (!Inserted) [[unlikely]] {
      llvm::report_fatal_error(
          "At " + llvm::Twine(locToString(Loc)) + ": " + ElemKind + " " +
          llvm::Twine(Elem->qualifier()) +
          " already registered! Previous definition was here: " +
          llvm::Twine(locToString(It->second->Loc)));
    }
  }

  template <TemplateString Name, const Category<true> *Cat>
  void registerCounter(
      Counter<true, Name, Cat> *Ctr,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Ctr, Counters, Cat, Name, "Counter", Loc);
  }

  template <TemplateString Name, const Category<true> *Cat>
  void registerMMCounter(
      MinMaxCounter<true, Name, Cat> *Ctr,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Ctr, MMCounters, Cat, Name, "MinMaxCounter", Loc);
  }

  template <TemplateString Name, const Category<true> *Cat>
  void registerHistogram(
      Histogram<true, Name, Cat> *Hist,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Hist, Histograms, Cat, Name, "Histogram", Loc);
  }

  template <TemplateString Name, const Category<true> *Cat>
  void registerTimer(
      Timer<true, Name, Cat> *Tm,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Tm, Timers, Cat, Name, "Timer", Loc);
  }

  llvm::DenseMap<const Category<true> *,
                 llvm::DenseMap<llvm::StringRef, detail::CounterBase *>>
      Counters;

  llvm::DenseMap<const Category<true> *,
                 llvm::DenseMap<llvm::StringRef, detail::MinMaxCounterBase *>>
      MMCounters;

  llvm::DenseMap<const Category<true> *,
                 llvm::DenseMap<llvm::StringRef, detail::HistogramBase *>>
      Histograms;

  llvm::DenseMap<const Category<true> *,
                 llvm::DenseMap<llvm::StringRef, detail::TimerBase *>>
      Timers;

  llvm::StringMap<const Category<true> *> RegisteredCategories;
};

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
inline Counter<Enabled, Name, Cat>::Counter(std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  Registry::instance().registerCounter(this, Loc);
}

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
inline MinMaxCounter<Enabled, Name, Cat>::MinMaxCounter(
    std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  Registry::instance().registerMMCounter(this, Loc);
}

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
inline Histogram<Enabled, Name, Cat>::Histogram(
    std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  Registry::instance().registerHistogram(this, Loc);
}

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
inline Timer<Enabled, Name, Cat>::Timer(std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  Registry::instance().registerTimer(this, Loc);
}

/// \brief Prints the measured data from all registered categories into the
/// given output stream
void printMeasuredData(llvm::raw_ostream &OS);

/// \brief Prints the measured data from the given category into the given
/// output stream
void printMeasuredData(llvm::raw_ostream &OS, const pamm::Category<true> &Cat);
inline void printMeasuredData(llvm::raw_ostream &OS,
                              const pamm::Category<false> &Cat) {}

[[nodiscard]] inline ptrdiff_t
getSumCount(pamm::detail::IsCounter auto const &...Counters) noexcept {
  return (Counters.value() + ...);
}

} // namespace pamm

/// This class offers functionality to measure different performance metrics.
/// All relevant functions are wrapped into preprocessor macros and should only
/// be used through those macros. Using these macros allows us to disable PAMM
/// completely when no performance evaluation is needed. Macros are defined
/// in @see PAMMMacros.h
///
/// Currently, PAMM can be run with all metrics (severity level 2 = Full) or
/// only core metrics (severity level 1 = Core) enabled. The severity level can
/// be changed when building PhASAR. The CMake option is
/// -DPHASAR_ENABLE_PAMM=[Off/Core/Full]. Note that PAMM will be disabled
/// (severity level 0 = Off) when building and running unittests.
///
/// For better compile times it is advised to include @see PAMMMacros.h instead
/// of PAMM.h.
///
/// @brief This class offers functionality to assist a performance analysis of
/// the PhASAR framework.
/// @note This class implements the Singleton Pattern - use the
/// PAMM_GET_INSTANCE macro to retrieve an instance of PAMM before you use any
/// other macro from this class.
class [[deprecated("Use the new PAMM functionality from the psr::pamm "
                   "namespace instead")]] PAMM final {
public:
  using TimePoint_t = std::chrono::steady_clock::time_point;
  using Duration_t = std::chrono::nanoseconds;

  PAMM() noexcept = default;
  ~PAMM() = default;
  // PAMM is used as singleton.
  PAMM(const PAMM &PM) = delete;
  PAMM(PAMM &&PM) = delete;
  PAMM &operator=(const PAMM &PM) = delete;
  PAMM &operator=(PAMM &&PM) = delete;

  /// \brief Returns a reference to the PAMM object (singleton) - associated
  /// macro: PAMM_GET_INSTANCE.
  [[nodiscard]] static PAMM &getInstance();

  [[nodiscard]] static ptrdiff_t
  getSumCount(pamm::detail::IsCounter auto const &...Counters) {
    return (Counters.value() + ...);
  }

  void printTimers(llvm::raw_ostream &OS);

  void printCounters(llvm::raw_ostream &OS);

  void printHistograms(llvm::raw_ostream &OS);

  /// \brief Prints the measured data to the commandline
  void printMeasuredData(llvm::raw_ostream &OS) { pamm::printMeasuredData(OS); }
  void printMeasuredData(llvm::raw_ostream &OS,
                         const pamm::Category<true> &Cat) {
    pamm::printMeasuredData(OS, Cat);
  }
};

} // namespace psr

inline constexpr psr::pamm::Category PAMMCategory{"<global>"};
