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

#include <chrono>        // high_resolution_clock::time_point, milliseconds
#include <iosfwd>        // ostream
#include <set>           // set
#include <string>        // string
#include <unordered_map> // unordered_map
#include <vector>        // vector

namespace psr {

/**
 * This class offers functionality to measure different performance metrics.
 * All relevant functions are wrapped into preprocessor macros and should only
 * be used through those macros. Using these macros allows us to disable PAMM
 * completely when no performance evaluation is needed. Macros are defined
 * in @see PAMMMacros.h
 *
 * Currently, PAMM can be run with all metrics (severity level 2 = Full) or only
 * core metrics (severity level 1 = Core) enabled. The severity level can be
 * changed when building PhASAR. The CMake option is
 * -DPHASAR_ENABLE_PAMM=[Off/Core/Full]. Note that PAMM will be disabled
 * (severity level 0 = Off) when building and running unittests.
 *
 * For better compile times it is advised to include @see PAMMMacros.h instead
 * of PAMM.h.
 *
 * @brief This class offers functionality to assist a performance analysis of
 * the PhASAR framework.
 * @note This class implements the Singleton Pattern - use the PAMM_GET_INSTANCE
 * macro to retrieve an instance of PAMM before you use any other macro from
 * this class.
 */
class PAMM {
private:
  PAMM() = default;
  ~PAMM() = default;
  using TimePoint_t = std::chrono::high_resolution_clock::time_point;
  using Duration_t = std::chrono::milliseconds;
  std::unordered_map<std::string, TimePoint_t> RunningTimer;
  std::unordered_map<std::string, std::pair<TimePoint_t, TimePoint_t>>
      StoppedTimer;
  std::unordered_map<std::string,
                     std::vector<std::pair<TimePoint_t, TimePoint_t>>>
      RepeatingTimer;
  std::unordered_map<std::string, unsigned> Counter;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, unsigned long>>
      Histogram;

public:
  /// PAMM is used as singleton.
  PAMM(const PAMM &pm) = delete;
  PAMM(PAMM &&pm) = delete;
  PAMM &operator=(const PAMM &pm) = delete;
  PAMM &operator=(PAMM &&pm) = delete;

  /**
   * @brief Returns a reference to the PAMM object (singleton) - associated
   * macro: PAMM_GET_INSTANCE.
   */
  static PAMM &getInstance();

  /**
   * @brief Resets PAMM, i.e. discards all gathered information (timer, counter
   * etc.) - associated macro: RESET_PAMM.
   * @note Only used for unit testing to reset PAMM in between test runs.
   */
  void reset();

  /**
   * @brief Starts a timer under the given timer id - associated macro:
   * START_TIMER(TIMER_ID, SEV_LVL).
   * @param TimerId Unique timer id.
   */
  void startTimer(const std::string &TimerId);

  /**
   * @brief Resets timer under the given timer id - associated macro:
   * RESET_TIMER(TIMER_ID, SEV_LVL).
   * @param TimerId Unique timer id.
   */
  void resetTimer(const std::string &TimerId);

  /**
   * If pauseTimer is true, a running timer gets paused, its start time point
   * will paired with a current time point, and stored as an accumulated timer.
   * This enables us to repeatedly compute execution time for a certain portion
   * of code which is executed multiple times, e.g. a loop or a function
   * call, without using a different timer id for every time computation.
   * Times of all executions of one timer are saved as distinct time point
   * pairs. Associated macro:
   *    PAUSE_TIMER(TIMER_ID, SEV_LVL)
   *
   * Otherwise, the timer will be simply stopped. Associated macro:
   *    STOP_TIMER(TIMER_ID, SEV_LVL)
   * @brief Stops or pauses a timer under the given timer id.
   * @param TimerId Unique timer id.
   * @param PauseTimer If true, timer will be paused instead of stopped.
   */
  void stopTimer(const std::string &TimerId, bool PauseTimer = false);

  /**
   * @brief Computes the elapsed time of the given timer up until now or up to
   * the moment the timer was stopped - associated macro: GET_TIMER(TIMERID)
   * @param TimerId Unique timer id.
   * @return Timer duration.
   */
  unsigned long elapsedTime(const std::string &TimerId);

  /**
   * For each accumulated timer a vector holds all recorded durations.
   * @brief Computes the elapsed time for all accumulated timer being used.
   * @return Map containing measured durations of all accumulated timer.
   */
  std::unordered_map<std::string, std::vector<unsigned long>>
  elapsedTimeOfRepeatingTimer();

  /**
   * A running timer will not be stopped. The precision for time computation
   * is set to milliseconds and the output is similar to a timestamp, e.g.
   * '4h 8m 15sec 16ms'.
   *
   * Associated macro PRINT_TIMER(TIMERID) does not check PAMM's severity level
   * explicitly.
   * @brief Returns the elapsed time for a given timer id.
   * @param timerId Unique timer id.
   */
  static std::string getPrintableDuration(unsigned long Duration);

  /**
   * @brief Registers a new counter under the given counter id - associated
   * macro: REG_COUNTER(COUNTER_ID, INIT_VALUE, SEV_LVL).
   * @param CounterId Unique counter id.
   */
  void regCounter(const std::string &CounterId, unsigned IntialValue = 0);

  /**
   * @brief Increases the count for the given counter - associated macro:
   * INC_COUNTER(COUNTER_ID, VALUE, SEV_LVL).
   * @param CounterId Unique counter id.
   * @param CValue to be added to the current counter.
   */
  void incCounter(const std::string &CounterId, unsigned CValue = 1);

  /**
   * @brief Decreases the count for the given counter - associated macro:
   * DEC_COUNTER(COUNTER_ID, VALUE, SEV_LVL).
   * @param CounterId Unique counter id.
   * @param CValue to be subtracted from the current counter.
   */
  void decCounter(const std::string &CounterId, unsigned CValue = 1);

  /**
   * The associated macro does not check PAMM's severity level explicitly.
   * @brief Returns the current count for the given counter - associated macro:
   * GET_COUNTER(COUNTER_ID).
   * @param CounterId Unique counter id.
   */
  int getCounter(const std::string &CounterId);

  /**
   * The associated macro does not check PAMM's severity level explicitly.
   * @brief Sums the counts for the given counter ids - associated macro:
   * GET_SUM_COUNT(...).
   * @param CounterIds Unique counter ids.
   * @note Macro uses variadic parameters, e.g. GET_SUM_COUNT({"foo", "bar"}).
   */
  int getSumCount(const std::set<std::string> &CounterIds);

  /**
   * @brief Registers a new histogram - associated macro:
   * REG_HISTOGRAM(HISTOGRAM_ID, SEV_LVL).
   * @param HistogramId Unique hitogram id.
   */
  void regHistogram(const std::string &HistogramId);

  /**
   * @brief Adds a new observed data point to the corresponding histogram -
   * associated macro: ADD_TO_HISTOGRAM(HISTOGRAM_ID, DATAPOINT_ID,
   * DATAPOINT_VALUE, SEV_LVL).
   * @param HistogramId ID of the histogram that tracks given data points.
   * @param DataPointId ID of the given data point.
   * @param DataPointValue Value of the given data point.
   */
  void addToHistogram(const std::string &HistogramId,
                      const std::string &DataPointId,
                      unsigned long DataPointValue = 1);

  void printTimers(std::ostream &os);

  void printCounters(std::ostream &os);

  void printHistograms(std::ostream &os);

  /**
   * @brief Prints the measured data to the commandline - associated macro:
   * PRINT_MEASURED_DATA
   */
  void printMeasuredData(std::ostream &os);

  /**
   * @brief Exports the measured data to JSON - associated macro:
   * EXPORT_MEASURED_DATA(PATH).
   * @param OutputPath to exported JSON file.
   */
  void exportMeasuredData(std::string OutputPath);
};

} // namespace psr

#endif
