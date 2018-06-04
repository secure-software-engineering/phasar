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

#ifndef PAMM_H_
#define PAMM_H_

#include "json.hpp"
#include <boost/filesystem.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <gtest/gtest_prod.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

// for convenience
using json = nlohmann::json;
namespace bfs = boost::filesystem;

namespace psr{

#ifdef PERFORMANCE_EVA
#define PAMM_FACTORY PAMM &pamm = PAMM::getInstance()
#define PAMM_RESET pamm.reset()
#define START_TIMER(TIMERID) pamm.startTimer(TIMERID)
#define RESET_TIMER(TIMERID) pamm.resetTimer(TIMERID)
#define PAUSE_TIMER(TIMERID) pamm.stopTimer(TIMERID, true)
#define STOP_TIMER(TIMERID) pamm.stopTimer(TIMERID)
#define GET_TIMER(TIMERID) pamm.elapsedTime(TIMERID)
#define ACC_TIMER(...) pamm.getPrintableDuration(accumulatedTime(__VA_ARGS__))
#define PRINT_TIMER(TIMERID)                                                   \
  pamm.getPrintableDuration(pamm.elapsedTime(TIMERID))
#define REG_COUNTER(COUNTERID) pamm.regCounter(COUNTERID)
#define REG_COUNTER_WITH_VALUE(COUNTERID, VALUE)                               \
  pamm.regCounter(COUNTERID, VALUE)
#define INC_COUNTER(COUNTERID) pamm.incCounter(COUNTERID)
#define INC_COUNTER_BY_VAL(COUNTERID, VAL) pamm.incCounter(COUNTERID, VAL)
#define DEC_COUNTER(COUNTERID) pamm.decCounter(COUNTERID)
#define GET_COUNTER(COUNTERID) pamm.getCounter(COUNTERID)
#define GET_SUM_COUNT(...) pamm.getSumCount(__VA_ARGS__)
#define REG_HISTOGRAM(HID) pamm.regHistogram(HID)
#define ADD_TO_HIST(HID, VAL) pamm.addToHistogram(HID, std::to_string(VAL))
#define ADD_TO_HIST_WITH_OCC(HID, VAL, OCC) pamm.addToHistogram(HID, std::to_string(VAL), OCC)
#define PRINT_EVA_DATA pamm.printData()
#define EXPORT_EVA_DATA(CONFIG) pamm.exportDataAsJSON(CONFIG)
#else
#define PAMM_FACTORY
#define PAMM_RESET
#define START_TIMER(TIMERID)
#define RESET_TIMER(TIMERID)
#define PAUSE_TIMER(TIMERID)
#define STOP_TIMER(TIMERID)
#define GET_TIMER(TIMERID)
#define ACC_TIMER(...)
#define PRINT_TIMER(TIMERID)
#define REG_COUNTER(COUNTERID)
#define REG_COUNTER_WITH_VALUE(COUNTERID, VALUE)
#define INC_COUNTER(COUNTERID)
#define INC_COUNTER_BY_VAL(COUNTERID, VAL)
#define DEC_COUNTER(COUNTERID)
#define GET_COUNTER(COUNTERID)
#define GET_SUM_COUNT(...)
#define REG_HISTOGRAM(HID)
#define ADD_TO_HIST(HID, VAL)
#define ADD_TO_HIST_WITH_OCC(HID, VAL, OCC)
#define PRINT_EVA_DATA
#define EXPORT_EVA_DATA(CONFIG)
#endif

/**
 * This class offers functionality to measure different performance metrics.
 * All main functions are wrapped into preprocessor macros and should only be
 * used by those. The macros can be enabled by passing the
 * -PERFORMANCE_EVA flag when compiling the framework. Other functions should
 * only be used within explicit macro guards:
 *    #ifdef PERFORMANCE_EVA
 *      ...
 *    #endif
 *
 * @brief This class offers functionality to assist a performance analysis of
 * the PHASAR framework.
 * @note This class implements the Singleton Pattern - use the PAMM_FACTORY
 * macro to retrieve an instance of PAMM before you use any other macro from
 * this class.
 */
class PAMM {
private:
  PAMM() = default;
  ~PAMM() = default;
  typedef std::chrono::high_resolution_clock::time_point time_point;
  std::unordered_map<std::string, time_point> RunningTimer;
  std::unordered_map<std::string, std::pair<time_point, time_point>>
      StoppedTimer;
  std::unordered_map<std::string,
                     std::vector<std::pair<time_point, time_point>>>
      AccumulatedTimer;
  std::unordered_map<std::string, unsigned> Counter;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, unsigned long>>
      Histogram;

  // for test purpose only
  unsigned long getHistoData(std::string HID, std::string VAL);

  template <typename Period = std::chrono::milliseconds>
  void printStoppedTimer() {
    std::cout << "Stopped timer\n";
    for (auto entry : StoppedTimer) {
      std::cout << entry.first << " : " << elapsedTime<Period>(entry.first) << '\n';
    }
  }

  // friend tests
  // FRIEND_TEST(PAMMTest, HandleSetHisto);
  // FRIEND_TEST(PAMMTest, PerformanceTimerPAMM);

public:
  /// PAMM is used as singleton.
  PAMM(const PAMM &pm) = delete;
  PAMM(PAMM &&pm) = delete;
  PAMM &operator=(const PAMM &pm) = delete;
  PAMM &operator=(PAMM &&pm) = delete;

  /**
   * @brief Returns a reference to the PAMM object (singleton) - associated
   * macro: PAMM_FACTORY.
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
   * START_TIMER(TIMERID).
   * @param timerId Unique timer id.
   */
  void startTimer(std::string timerId);

  /**
   * @brief Resets timer under the given timer id - associated macro:
   * RESET_TIMER(TIMERID).
   * @param timerId Unique timer id.
   */
  void resetTimer(std::string timerId);

  /**
   * If pauseTimer is true, a running timer gets paused, its start time point
   * will paired with
   * a current time point, and stored as an accumulated timer. This
   * enables us to repeatedly compute execution time for a certain portion of
   * code which is executed multiple times, e.g. a loop or a function
   * call, without using a different timer id for every time computation.
   * Times of all executions of one timer are saved as distinct time point
   * pairs. Associated macro:
   *    PAUSE_TIMER(TIMERID)
   *
   * Otherwise, the timer will be simply stopped. Associated macro:
   *    STOP_TIMER(TIMERID)
   * @brief Stops or pauses a timer under the given timer id.
   * @param timerId Unique timer id.
   * @param pauseTimer If true, timer will be paused instead of stopped.
   */
  void stopTimer(std::string timerId, bool pauseTimer = false);

  /**
   * @brief Computes the elapsed time of the given timer up until now or up to
   * the
   * moment the timer was stopped - associated macro: GET_TIMER(TIMERID)
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   * @param timerId Unique timer id.
   * @return Duration with respect to the Period.
   * @note When using the macro, the period is set to milliseconds and cannot be
   * customized by the macro.
   */
  template <typename Period = std::chrono::milliseconds>
  unsigned long elapsedTime(std::string timerId) {
    assert((RunningTimer.count(timerId) || StoppedTimer.count(timerId)) &&
           "elapsedTime failed due to an invalid timer id");
    if (RunningTimer.count(timerId)) {
      time_point end = std::chrono::high_resolution_clock::now();
      time_point start = RunningTimer[timerId];
      auto duration = std::chrono::duration_cast<Period>(end - start);
      return duration.count();
    } else if (StoppedTimer.count(timerId)) {
      auto duration = std::chrono::duration_cast<Period>(
          StoppedTimer[timerId].second - StoppedTimer[timerId].first);
      return duration.count();
    }
    return 0;
  }

  /**
   * @brief Computes the accumulated time of the given timer ids - associated
   * macro: ACC_TIMER(...)
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   * @param timers Unique timer ids.
   * @return Accumulated elapsed time.
   * @note Macro uses variadic parameters, e.g. ACC_TIMER({"foo", "bar"}).
   */
  template <typename Period = std::chrono::milliseconds>
  unsigned long accumulatedTime(std::set<std::string> timers) {
    unsigned long accTime = 0;
    for (auto timerId : timers) {
      accTime += elapsedTime<Period>(timerId);
    }
    return accTime;
  }

  /**
   * @brief Computes the elapsed time for all accumulated timer.
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   * @return Map containing all timer durations.
   */
  template <typename Period = std::chrono::milliseconds>
  std::unordered_map<std::string, std::vector<unsigned long>>
  elapsedTimeForAccTimer() {
    std::unordered_map<std::string, std::vector<unsigned long>> accTimes;
    for (auto timer : AccumulatedTimer) {
      std::vector<unsigned long> accTimeVec;
      for (auto timepair : timer.second) {
        auto duration = std::chrono::duration_cast<Period>(timepair.second -
                                                           timepair.first);
        accTimeVec.push_back(duration.count());
      }
      accTimes[timer.first] = accTimeVec;
    }
    return accTimes;
  }

  /**
   * A running timer will not be stopped. The precision for time computation
   * is set to milliseconds and the output is similar to a timestamp, e.g.
   * '4h 8m 15sec 16ms'.
   *
   * @brief Returns the elapsed time for a given timer id - associated macro:
   * PRINT_TIMER(TIMERID).
   * @param timerId Unique timer id.
   */
  std::string getPrintableDuration(unsigned long duration);

  /**
   * @brief Registers a new counter under the given counter id - associated
   * macro: REG_COUNTER(COUNTERID).
   * @param counterId Unique counter id.
   */
  void regCounter(std::string counterId, unsigned intialValue = 0);

  /**
   * @brief Increases the count for the given counter - associated macro:
   * INC_COUNTER(COUNTERID).
   * @param counterId Unique counter id.
   * @param value to be added to the current counter count.
   */
  void incCounter(std::string counterId, unsigned value = 1);

  /**
   * @brief Decreases the count for the given counter - associated macro:
   * DEC_COUNTER(COUNTERID).
   * @param counterId Unique counter id.
   * @param value to be subtracted from the current counter count.
   */
  void decCounter(std::string counterId, unsigned value = 1);

  /**
   * @brief Returns the current counter count - associated macro:
   * GET_COUNTER(COUNTERID).
   * @param counterId Unique counter id.
   */
  int getCounter(std::string counterId);

  /**
   * @brief Sums the counts of the given counter ids - associated macro:
   * GET_SUM_COUNT(...).
   * @param counterId Unique counter ids.
   * @note Macro uses variadic parameters, e.g. GET_SUM_COUNT({"foo", "bar"}).
   */
  int getSumCount(std::set<std::string> counterIds);

  /**
   * @brief Registers a new set as a set histogram.
   * @param HID identifies the particular set.
   */
  void regHistogram(std::string HID);

  /**
   * @brief Adds a new observed set size to the corresponding set histogram.
   * @param HID ID of the set.
   * @param OCC the added value.
   */
  void addToHistogram(std::string HID, std::string VAL, unsigned long OCC = 1);

  void printCounters();

  void printHistograms();
  /**
   * @brief Prints the measured data to the command line - associated macro:
   * PRINT_EVA_DATA
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   */
  template <typename Period = std::chrono::milliseconds> void printData() {
    // stop all running timer
    while (!RunningTimer.empty()) {
      stopTimer(RunningTimer.begin()->first);
    }
    std::cout << "\n----- START OF EVALUATION DATA -----\n\n";
    std::cout << "Single Timer\n";
    std::cout << "------------\n";
    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      std::cout << timer.first << " : "
                << getPrintableDuration(time) << '\n';
    }
    if (StoppedTimer.empty()) {
      std::cout << "No single Timer started!\n\n";
    } else {
      std::cout << "\n";
    }
    std::cout << "Accumulated Timer\n";
    std::cout << "-----------------\n";
    for (auto timer : elapsedTimeForAccTimer()) {
      unsigned long sum = 0;
      std::cout << timer.first << " Timer:\n";
      for (auto duration : timer.second) {
        sum += duration;
        std::cout << duration << '\n';
      }
      std::cout << "===\n" << sum << "\n\n";
    }
    if (AccumulatedTimer.empty()) {
      std::cout << "No accumulated Timer started!\n";
    } else {
      std::cout << '\n';
    }
    printCounters();
    printHistograms();
    std::cout << "\n----- END OF EVALUATION DATA -----\n\n";
  }

  /**
   * @brief Computes misc times of the analysis/IR preprocssing/framework
   * runtime, i.e. the remaining time that is not explicitly measured by a
   * timer.
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   * @return Set containing the computed misc times.
   */
  template <typename Period = std::chrono::milliseconds>
  std::set<std::pair<std::string, unsigned long>> computeMiscTimes() {
    std::set<std::pair<std::string, unsigned long>> miscTimes;
    unsigned long fw_runtime, dfa_runtime;
    // these checks are only needed to enable PAMM Tests that do not use
    // one of those two timers
    if (RunningTimer.count("DFA Runtime") ||
        StoppedTimer.count("DFA Runtime")) {
      dfa_runtime = elapsedTime<Period>("DFA Runtime");
    }
    if (RunningTimer.count("FW Runtime") || StoppedTimer.count("FW Runtime")) {
      fw_runtime = elapsedTime<Period>("FW Runtime");
    }

    for (auto timer : StoppedTimer) {
      if (timer.first.find("DFA") != std::string::npos &&
          timer.first != "DFA Runtime") {
        dfa_runtime -= elapsedTime<Period>(timer.first);
      } else if (timer.first != "FW Runtime") {
        fw_runtime -= elapsedTime<Period>(timer.first);
      }
    }
    miscTimes.insert(std::make_pair("DFA Misc", dfa_runtime));
    miscTimes.insert(std::make_pair("FW Misc", fw_runtime));
    return miscTimes;
  }

  /**
   * @brief Exports the measured data to JSON - associated macro:
   * EXPORT_EVA_DATA(CONFIG).
   * @tparam Period sets the precision for time computation, milliseconds by
   * default.
   * @param config name.
   */
  template <typename Period = std::chrono::milliseconds>
  void exportDataAsJSON(const std::string &configPath) {
    // main json file
    json jsonData;
    // stop all running timer
    while (!RunningTimer.empty()) {
      stopTimer(RunningTimer.begin()->first);
    }
    json jTimer;
    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      jTimer[timer.first] = time;
    }
    // compute misc times
    for (auto miscTime : computeMiscTimes<Period>()) {
      jTimer[miscTime.first] = miscTime.second;
    }
    jsonData["Timer"] = jTimer;
    for (auto timer : elapsedTimeForAccTimer()) {
      jsonData[timer.first + " Times"] = timer.second;
    }
    addHistogramToJSON(jsonData);
    addCounterToJSON(jsonData);
    bfs::path cfp(configPath);
    // reduce the config path to just the filename - no path and no extension
    std::string config = cfp.filename().string();
    std::size_t extensionPos = config.find(cfp.extension().string());
    config.replace(extensionPos, cfp.extension().size(), "");
    jsonData["Config"] = config;
    jsonData["Config path"] = configPath;
    std::ofstream file(config + ".json");
    file << std::setw(2) // sets the indentation
         << jsonData << std::endl;
    file.close();
  }

  /**
   * @brief Adds the Set Histograms in a more organized way to
   * the main json file.
   * @param jsonData Main json file to append to.
   */
  void addHistogramToJSON(json &jsonData);

  /**
   * @brief Adds the Counters in a more organized way to
   * the main json file.
   * @param jsonData Main json file to append to.
   */
  void addCounterToJSON(json &jsonData);

  // for test purpose only

  void printStoppedTimer();
};

}//namespace psr

#endif /* PAMM_H_ */
