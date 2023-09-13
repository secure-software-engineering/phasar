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

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <chrono> // high_resolution_clock::time_point, milliseconds
#include <initializer_list>
#include <optional>
#include <set>    // set
#include <string> // string
#include <vector> // vector

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

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
  using TimePoint_t = std::chrono::high_resolution_clock::time_point;
  using Duration_t = std::chrono::milliseconds;

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
  /// is set to milliseconds and the output is similar to a timestamp, e.g.
  /// '4h 8m 15sec 16ms'.
  ///
  /// Associated macro PRINT_TIMER(TIMERID) does not check PAMM's severity level
  /// explicitly.
  /// \brief Returns the elapsed time for a given timer id.
  /// \param timerId Unique timer id.
  [[nodiscard]] static std::string getPrintableDuration(uint64_t Duration);

  /// \brief Registers a new counter under the given counter id - associated
  /// macro: REG_COUNTER(COUNTER_ID, INIT_VALUE, SEV_LVL).
  /// \param CounterId Unique counter id.
  void regCounter(llvm::StringRef CounterId, unsigned IntialValue = 0);

  /// \brief Increases the count for the given counter - associated macro:
  /// INC_COUNTER(COUNTER_ID, VALUE, SEV_LVL).
  /// \param CounterId Unique counter id.
  /// \param CValue to be added to the current counter.
  void incCounter(llvm::StringRef CounterId, unsigned CValue = 1);

  /// \brief Decreases the count for the given counter - associated macro:
  /// DEC_COUNTER(COUNTER_ID, VALUE, SEV_LVL).
  /// \param CounterId Unique counter id.
  /// \param CValue to be subtracted from the current counter.
  void decCounter(llvm::StringRef CounterId, unsigned CValue = 1);

  /// The associated macro does not check PAMM's severity level explicitly.
  /// \brief Returns the current count for the given counter - associated macro:
  /// GET_COUNTER(COUNTER_ID).
  /// \param CounterId Unique counter id.
  std::optional<unsigned> getCounter(llvm::StringRef CounterId);

  /// The associated macro does not check PAMM's severity level explicitly.
  /// \brief Sums the counts for the given counter ids - associated macro:
  /// GET_SUM_COUNT(...).
  /// \param CounterIds Unique counter ids.
  /// \note Macro uses variadic parameters, e.g. GET_SUM_COUNT({"foo", "bar"}).
  std::optional<uint64_t> getSumCount(const std::set<std::string> &CounterIds);
  std::optional<uint64_t>
  getSumCount(llvm::ArrayRef<llvm::StringRef> CounterIds);
  std::optional<uint64_t>
  getSumCount(std::initializer_list<llvm::StringRef> CounterIds);

  /// \brief Registers a new histogram - associated macro:
  /// REG_HISTOGRAM(HISTOGRAM_ID, SEV_LVL).
  /// \param HistogramId Unique hitogram id.
  void regHistogram(llvm::StringRef HistogramId);

  /// \brief Adds a new observed data point to the corresponding histogram -
  /// associated macro: ADD_TO_HISTOGRAM(HISTOGRAM_ID, DATAPOINT_ID,
  /// DATAPOINT_VALUE, SEV_LVL).
  /// \param HistogramId ID of the histogram that tracks given data points.
  /// \param DataPointId ID of the given data point.
  /// \param DataPointValue Value of the given data point.
  void addToHistogram(llvm::StringRef HistogramId, llvm::StringRef DataPointId,
                      uint64_t DataPointValue = 1);

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
  llvm::StringMap<unsigned> Counter;
  llvm::StringMap<llvm::StringMap<uint64_t>> Histogram;
};

} // namespace psr

#endif
