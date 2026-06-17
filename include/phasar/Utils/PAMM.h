/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PAMM.h
 *
 *  Created on: 06.12.2017
 *      Author: rleer
 */

#ifndef PHASAR_UTILS_PAMM_H_
#define PHASAR_UTILS_PAMM_H_

#include "phasar/Config/phasar-config.h"
#include "phasar/Utils/TemplateString.h"
#include "phasar/Utils/TypeTraits.h"

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
#include <vector> // vector

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

namespace pamm {

class Registry;
class Category {
public:
  explicit constexpr Category(llvm::StringLiteral Name,
                              bool IsEnabled = true) noexcept
      : Name(Name)
#if defined(PAMM_FULL) || defined(PAMM_CORE)
        ,
        IsEnabled(IsEnabled)
#endif
  {
  }

  constexpr operator llvm::StringRef() const noexcept { return Name; }

  [[nodiscard]] constexpr llvm::StringRef name() const noexcept { return Name; }

  [[nodiscard]] auto isEnabled() const noexcept { return IsEnabled; }

  void disable() noexcept {
#if defined(PAMM_FULL) || defined(PAMM_CORE)
    IsEnabled = false;
#endif
  }

  void enable() noexcept {
#if defined(PAMM_FULL) || defined(PAMM_CORE)
    IsEnabled = true;
#else
    llvm::WithColor::warning()
        << "Cannot enable PAMM category '" << Name
        << "', because PAMM is disabled at compile time\n";
#endif
  }

private:
  llvm::StringRef Name;
#if defined(PAMM_FULL) || defined(PAMM_CORE)
  bool IsEnabled = true;
#else
  [[no_unique_address]] std::false_type IsEnabled{};
#endif
};

namespace detail {

struct CounterBase {
  ptrdiff_t Ctr{};
  Category *TheCategory{};
  std::source_location Loc{};
  // TODO: Add thread-safe counter
};
struct HistogramBase {
  llvm::StringMap<uint64_t> HistData{};
  Category *TheCategory{};
  std::source_location Loc{};
};
} // namespace detail

class Registry;

template <bool Enabled, TemplateString Name, Category *Cat>
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
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  [[nodiscard]] constexpr ptrdiff_t value() const noexcept { return Ctr; }

private:
  [[nodiscard]] constexpr detail::CounterBase *base() noexcept { return this; }
};

template <TemplateString Name, Category *Cat> class Counter<false, Name, Cat> {
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
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  [[nodiscard]] constexpr std::nullopt_t value() const noexcept {
    return std::nullopt;
  }
};

namespace detail {
struct IsCounterImpl {
  template <bool Enabled, TemplateString Name, Category *Cat>
  static std::true_type test(Counter<Enabled, Name, Cat>);

  static std::false_type test(...);
};

template <typename T>
concept IsCounter = decltype(IsCounterImpl::test(std::declval<T>()))::value;
} // namespace detail

template <bool Enabled, TemplateString Name, Category *Cat>
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
    Ret += llvm::StringRef(Name);
    return Ret;
  }

  void add(llvm::StringRef DataPointId, uint64_t Increment) {
    this->HistData[DataPointId] += Increment;
  }
  template <has_adl_to_string_v T>
    requires(!std::convertible_to<T, llvm::StringRef>)
  void add(T &&DataPointId, uint64_t Increment) {
    this->HistData[psr::adl_to_string(PSR_FWD(DataPointId))] += Increment;
  }

private:
  [[nodiscard]] constexpr detail::HistogramBase *base() noexcept {
    return this;
  }
};

template <TemplateString Name, Category *Cat>
class Histogram<false, Name, Cat> {
  friend Registry;

public:
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Histogram /*H*/) {
    return OS << Cat->name() << "::" << Name << "\n";
  }

  [[nodiscard]] constexpr std::string qualifier() const {
    auto Ret = std::string(Cat->name());
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

class Registry {
  template <bool Enabled, TemplateString Name, Category *Cat>
  friend class Counter;
  template <bool Enabled, TemplateString Name, Category *Cat>
  friend class Histogram;

public:
  static Registry &instance() {
    static Registry Reg;
    return Reg;
  }

  void printCounters(llvm::raw_ostream &OS) const;
  void printHistograms(llvm::raw_ostream &OS) const;

private:
  static void registerImpl(auto *Elem, auto &Into, Category *Cat,
                           llvm::StringRef Name, llvm::StringRef ElemKind,
                           std::source_location Loc) {
    assert(Elem != nullptr);
    auto [It, Inserted] =
        Into[Cat].try_emplace(llvm::StringRef(Name), Elem->base());
    if (!Inserted) [[unlikely]] {
      llvm::report_fatal_error(
          "At " + llvm::Twine(Loc.file_name()) + ":" + llvm::Twine(Loc.line()) +
          ":" + llvm::Twine(Loc.column()) + ": " + ElemKind + " " +
          llvm::Twine(Elem->qualifier()) +
          " already registered! Previous definition was here: " +
          llvm::Twine(It->second->Loc.file_name()) + ":" +
          llvm::Twine(It->second->Loc.line()) + ":" +
          llvm::Twine(It->second->Loc.column()));
    }
  }

  template <TemplateString Name, Category *Cat>
  void registerCounter(
      Counter<true, Name, Cat> *Ctr,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Ctr, Counters, Cat, Name, "Counter", Loc);
  }

  template <TemplateString Name, Category *Cat>
  void registerHistogram(
      Histogram<true, Name, Cat> *Hist,
      std::source_location Loc = std::source_location::current()) noexcept {
    registerImpl(Hist, Histograms, Cat, Name, "Histogram", Loc);
  }

  llvm::DenseMap<Category *,
                 llvm::DenseMap<llvm::StringRef, detail::CounterBase *>>
      Counters;

  llvm::DenseMap<Category *,
                 llvm::DenseMap<llvm::StringRef, detail::HistogramBase *>>
      Histograms;
};

template <bool Enabled, TemplateString Name, Category *Cat>
inline Counter<Enabled, Name, Cat>::Counter(std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  this->TheCategory = Cat;
  Registry::instance().registerCounter(this, Loc);
}

template <bool Enabled, TemplateString Name, Category *Cat>
inline Histogram<Enabled, Name, Cat>::Histogram(
    std::source_location Loc) noexcept {
  static_assert(Cat != nullptr);
  this->Loc = Loc;
  this->TheCategory = Cat;
  llvm::errs() << "Registering histogram: " << Name << '\n';
  Registry::instance().registerHistogram(this, Loc);
}

} // namespace pamm

inline constexpr pamm::Category PAMMCategory{"<global>"};

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

  /// \brief Resets PAMM, i.e. discards all gathered information (timer, counter
  /// etc.) - associated macro: RESET_PAMM.
  /// \note Only used for unit testing to reset PAMM in between test runs.
  void reset();

  /// \brief Starts a timer under the given timer id - associated macro:
  /// START_TIMER(TIMER_ID, SEV_LVL).
  /// \param TimerId Unique timer id.
  void startTimer(llvm::StringRef TimerId);

  /// \brief Resets timer under the given timer id - associated macro:
  /// RESET_TIMER(TIMER_ID, SEV_LVL).
  /// \param TimerId Unique timer id.
  void resetTimer(llvm::StringRef TimerId);

  /// If pauseTimer is true, a running timer gets paused, its start time point
  /// will paired with a current time point, and stored as an accumulated timer.
  /// This enables us to repeatedly compute execution time for a certain portion
  /// of code which is executed multiple times, e.g. a loop or a function
  /// call, without using a different timer id for every time computation.
  /// Times of all executions of one timer are saved as distinct time point
  /// pairs. Associated macro:
  ///    PAUSE_TIMER(TIMER_ID, SEV_LVL)
  ///
  /// Otherwise, the timer will be simply stopped. Associated macro:
  ///    STOP_TIMER(TIMER_ID, SEV_LVL)
  /// \brief Stops or pauses a timer under the given timer id.
  /// \param TimerId Unique timer id.
  /// \param PauseTimer If true, timer will be paused instead of stopped.
  void stopTimer(llvm::StringRef TimerId, bool PauseTimer = false);

  /// \brief Computes the elapsed time of the given timer up until now or up to
  /// the moment the timer was stopped - associated macro: GET_TIMER(TIMERID)
  /// \param TimerId Unique timer id.
  /// \return Timer duration.
  uint64_t elapsedTime(llvm::StringRef TimerId);

  /// For each accumulated timer a vector holds all recorded durations.
  /// \brief Computes the elapsed time for all accumulated timer being used.
  /// \return Map containing measured durations of all accumulated timer.
  [[nodiscard]] llvm::StringMap<std::vector<uint64_t>>
  elapsedTimeOfRepeatingTimer();

  /// A running timer will not be stopped. The precision for time computation
  /// is set to microseconds and the output is of the form: HH:MM:SS:XXXXXX,
  /// where XXXXXX are 6 digits of sub-seconds.
  ///
  /// Associated macro PRINT_TIMER(TIMERID) does not check PAMM's severity level
  /// explicitly.
  /// \brief Returns the elapsed time for a given timer id.
  /// \param timerId Unique timer id.
  [[nodiscard]] static std::string getPrintableDuration(uint64_t Duration);

  [[nodiscard]] ptrdiff_t
  getSumCount(pamm::detail::IsCounter auto const &...Counters) {
    return (Counters.value() + ...);
  }

  void stopAllTimers();

  void printTimers(llvm::raw_ostream &OS);

  void printCounters(llvm::raw_ostream &OS);

  void printHistograms(llvm::raw_ostream &OS);

  /// \brief Prints the measured data to the commandline - associated macro:
  /// PRINT_MEASURED_DATA
  void printMeasuredData(llvm::raw_ostream &OS);

  /// \brief Exports the measured data to JSON - associated macro:
  /// EXPORT_MEASURED_DATA(PATH).
  /// \param OutputPath to exported JSON file.
  void exportMeasuredData(
      const llvm::Twine &OutputPath,
      llvm::StringRef ProjectId = "default-phasar-project",
      const std::vector<std::string> *Modules = nullptr,
      const std::vector<std::string> *DataFlowAnalyses = nullptr);

  [[nodiscard]] const auto &getHistogram() const noexcept { return Histogram; }

private:
  llvm::StringMap<TimePoint_t> RunningTimer;
  llvm::StringMap<std::pair<TimePoint_t, TimePoint_t>> StoppedTimer;
  llvm::StringMap<std::vector<std::pair<TimePoint_t, TimePoint_t>>>
      RepeatingTimer;
  llvm::StringMap<uint64_t> Counter;
  llvm::StringMap<llvm::StringMap<uint64_t>> Histogram;
};

} // namespace psr

#endif
