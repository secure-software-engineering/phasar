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
#include "phasar/Utils/Timer.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/WithColor.h"

#include <chrono> // high_resolution_clock::time_point, milliseconds
#include <concepts>
#include <cstddef>
#include <optional>
#include <source_location>
#include <string> // string
#include <type_traits>
#include <utility>

namespace llvm {
class raw_ostream;
} // namespace llvm

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

  [[nodiscard]] constexpr auto isEnabled() const noexcept { return IsEnabled; }

  constexpr void disable() noexcept { IsEnabled = false; }
  constexpr void enable() noexcept { IsEnabled = true; }

private:
  llvm::StringRef Name;
  bool IsEnabled = true;
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
struct CounterBase : PAMMBase {
  ptrdiff_t Ctr{};
  // TODO: Add thread-safe counter
};
struct HistogramBase : PAMMBase {
  llvm::StringMap<uint64_t> HistData{};
};
struct TimerBase : PAMMBase {
  std::optional<SimpleTimer> Tm{};
  std::chrono::nanoseconds Acc{};

  void start() noexcept {
    assert(!Tm.has_value() &&
           "Starting an already running timer is not allowed");
    Tm.emplace();
  }

  void stop() noexcept {
    assert(Tm.has_value() && "Stopping a not-running timer is not allowed");
    Acc += Tm->elapsedNanos();
    Tm.reset();
  }

  void reset() noexcept {
    Acc = {};
    Tm.reset();
  }

  [[nodiscard]] constexpr std::chrono::nanoseconds
  elapsedNanos() const noexcept {
    return Acc;
  }

  [[nodiscard]] hms elapsed() const noexcept { return Acc; }
};
} // namespace detail

class Registry;

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Counter : private detail::CounterBase {
  friend Registry;

public:
  inline explicit Counter(
      std::source_location Loc = std::source_location::current()) noexcept;

  constexpr void operator++() noexcept { ++Ctr; }
  constexpr void operator++(int) noexcept { ++Ctr; }
  constexpr void operator+=(ptrdiff_t Offset) noexcept { Ctr += Offset; }
  constexpr void operator-=(ptrdiff_t Offset) noexcept { Ctr -= Offset; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Counter C) {
    return OS << Cat->name() << "::" << Name << ": " << C.Ctr;
  }

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  [[nodiscard]] constexpr ptrdiff_t value() const noexcept { return Ctr; }

private:
  [[nodiscard]] constexpr detail::CounterBase *base() noexcept { return this; }
};

template <TemplateString Name, const Category<false> *Cat>
class Counter<false, Name, Cat> {
public:
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++() noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void operator++(int) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  operator+=(ptrdiff_t Offset) noexcept {}
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  operator-=(ptrdiff_t Offset) noexcept {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Counter /*C*/) {
    return OS << Cat->name() << "::" << Name;
  }

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  [[nodiscard]] constexpr std::nullopt_t value() const noexcept {
    return std::nullopt;
  }
};

namespace detail {
struct IsCounterImpl {
  template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
  static std::true_type test(Counter<Enabled, Name, Cat>);

  static std::false_type test(...);
};

template <typename T>
concept IsCounter = decltype(IsCounterImpl::test(std::declval<T>()))::value;
} // namespace detail

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Histogram : private detail::HistogramBase {
  friend Registry;

public:
  inline explicit Histogram(
      std::source_location Loc = std::source_location::current()) noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Histogram &H) {
    OS << Cat->name() << "::" << Name << ":\n";
    OS << "Value\t| #Occurrences\n";
    for (const auto &[Dat, Val] : H.HistData) {
      OS << Dat << "\t| " << Val << '\n';
    }
    return OS;
  }

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  void add(llvm::StringRef DataPointId, uint64_t Increment) {
    if (Cat->isEnabled()) {
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
class Histogram<false, Name, Cat> {
  friend Registry;

public:
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Histogram /*H*/) {
    return OS << Cat->name() << "::" << Name << "\n";
  }

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void
  add(llvm::StringRef /*DataPointId*/, uint64_t /*Increment*/) {}

  template <has_adl_to_string_v T>
    requires(!std::convertible_to<T, llvm::StringRef>)
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr void add(T && /*DataPointId*/,
                                                  uint64_t /*Increment*/) {}
};

template <bool Enabled, TemplateString Name, const Category<Enabled> *Cat>
class Timer : private detail::TimerBase {
  friend Registry;
  template <bool Enabled2> friend class ScopedTimer;

public:
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

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
  }

private:
  [[nodiscard]] constexpr detail::TimerBase *base() noexcept { return this; }
};

template <TemplateString Name, const Category<false> *Cat>
class Timer<false, Name, Cat> {
public:
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

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
    Ret += "::";
    Ret += llvm::StringRef(Name);
    return Ret;
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
    if (Tm->Tm.has_value()) {
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
class PAMM final {
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
