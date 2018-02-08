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

// for convenience
using json = nlohmann::json;
namespace bfs = boost::filesystem;

#ifdef PERFORMANCE_EVA
#define PAMM_FACTORY PAMM &pamm = PAMM::getInstance()
#define PAMM_RESET pamm.reset()
#define START_TIMER(TIMERID) pamm.startTimer(TIMERID)
#define RESET_TIMER(TIMERID) pamm.resetTimer(TIMERID)
#define STOP_TIMER(TIMERID) pamm.stopTimer(TIMERID)
#define GET_TIMER(TIMERID) pamm.elapsedTime(TIMERID)
#define ACC_TIMER(...) pamm.getPrintableDuration(accumulatedTime(__VA_ARGS__))
#define PRINT_TIMER(TIMERID)                                                   \
  pamm.getPrintableDuration(pamm.elapsedTime(TIMERID))
#define REG_COUNTER(COUNTERID) pamm.regCounter(COUNTERID)
#define INC_COUNTER(COUNTERID) pamm.incCounter(COUNTERID)
#define DEC_COUNTER(COUNTERID) pamm.decCounter(COUNTERID)
#define GET_COUNTER(COUNTERID) pamm.getCounter(COUNTERID)
#define GET_SUM_COUNT(...) pamm.getSumCount(__VA_ARGS__)
#define ADD_DATA_TO_SET_HIST(SETSIZE) pamm.addDataToSetHistogram(SETSIZE)
#define PRINT_EVA_DATA pamm.printData()
#define EXPORT_EVA_DATA(CONFIG) pamm.exportDataAsJSON(CONFIG)
#else
#define PAMM_FACTORY
#define PAMM_RESET
#define START_TIMER(TIMERID)
#define RESET_TIMER(TIMERID)
#define STOP_TIMER(TIMERID)
#define GET_TIMER(TIMERID)
#define ACC_TIMER(...)
#define PRINT_TIMER(TIMERID)
#define REG_COUNTER(COUNTERID)
#define INC_COUNTER(COUNTERID)
#define DEC_COUNTER(COUNTERID)
#define GET_COUNTER(COUNTERID)
#define GET_SUM_COUNT(...)
#define ADD_DATA_TO_SET_HIST(SETSIZE)
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
 * macro before you use any other macro from this class.
 */
class PAMM {
private:
  PAMM() = default;
  ~PAMM() = default;
  typedef std::chrono::high_resolution_clock::time_point time_point;
  // TODO: try unordered_map instead of map - maybe it's faster
  std::map<const std::string, time_point> RunningTimer;
  std::map<const std::string, std::pair<time_point, time_point>> StoppedTimer;
  std::map<const std::string, unsigned> Counter;
  std::map<unsigned long, unsigned long> SetHistogram;

  // for test purpose only
  unsigned long getSetHistoData(unsigned long setSize);

  // friend tests
  FRIEND_TEST(PAMMTest, HandleSetHisto);

public:
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
   * @brief Stops a timer under the given timer id - associated macro:
   * STOP_TIMER(TIMERID).
   * @param timerId Unique timer id.
   */
  void stopTimer(std::string timerId);

  /**
   * @brief Computes the elapsed time of the given timer up until now or up to
   * the
   * moment the timer was stopped - associated macro: GET_TIMER(TIMERID)
   * @tparam Period sets the precision for time computation, e.g. microseconds.
   * @param timerId Unique timer id.
   * @return Duration with respect to the Period.
   * @note When using the macro, the period is set to microseconds and cannot be
   * customized by the macro.
   */
  template <typename Period = std::chrono::microseconds>
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
   * @tparam Period sets the precision for time computation, e.g. microseconds.
   * @param timers Unique timer ids.
   * @return Accumulated elapsed time.
   * @note Macro uses variadic parameters, e.g. ACC_TIMER({"foo", "bar"}).
   */
  template <typename Period = std::chrono::microseconds>
  unsigned long accumulatedTime(std::set<std::string> timers) {
    unsigned long accTime = 0;
    for (auto timerId : timers) {
      accTime += elapsedTime<Period>(timerId);
    }
    return accTime;
  }

  /**
   * A running timer will not be stopped. The precision for time computation
   * is set to microseconds and the output is similar to a timestamp, e.g.
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
  void regCounter(std::string counterId);

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
   * The set histogram tracks the sizes of sets.
   *
   * @brief Increases the count for a given set size by 1 - associated macro:
   * ADD_DATA_TO_SET_HIST(SETSIZE).
   * @param setSize which counter will be increased.
   */
  void addDataToSetHistogram(unsigned long setSize);

  /**
   * @brief Prints the measured data to the command line - associated macro:
   * PRINT_EVA_DATA
   * @tparam Period sets the precision for time computation, microseconds by default.
   */
  template <typename Period = std::chrono::microseconds> void printData() {
    // stop all running timer
    for (auto timer : RunningTimer) {
      stopTimer(timer.first);
    }
    std::cout << "\n----- START OF EVALUATION DATA -----\n\n";
    std::cout << "Timer\n";
    std::cout << "-----\n";

    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      std::cout << timer.first << " : " << getPrintableDuration(time) << '\n';
    }
    if (StoppedTimer.empty()) {
      std::cout << "No Timer started!\n";
    }
    std::cout << "\nCounter\n";
    std::cout << "-------\n";
    for (auto counter : Counter) {
      std::cout << counter.first << " : " << counter.second << '\n';
    }
    if (Counter.empty()) {
      std::cout << "No Counter registered!\n";
    }
    std::cout << "\nSet Histogram\n";
    std::cout << "-------\n";
    if (!SetHistogram.empty()) {
      std::cout << "Size : Count\n";
      for (auto set : SetHistogram) {
        std::cout << set.first << " : " << set.second << "\n";
      }
    } else {
      std::cout << "No sets tracked!\n";
    }
    std::cout << "\n----- END OF EVALUATION DATA -----\n\n";
  }

  /**
   * @brief Exports the measured data to JSON - associated macro:
   * EXPORT_EVA_DATA(CONFIG).
   * @tparam Period sets the precision for time computation, milliseconds by default.
   * @param config name.
   */
  template <typename Period = std::chrono::milliseconds>
  void exportDataAsJSON(const std::string &configPath) {
    // stop all running timer
    for (auto timer : RunningTimer) {
      stopTimer(timer.first);
    }
    json jTimer;
    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      jTimer[timer.first] = time;
    }
    json jCounter(Counter);
    // json jSetHistogram(SetHistogram);
    json jSetHistogram;
    // TODO: json won't create a proper map when int/unsigned is used as a key
    for (auto set : SetHistogram) {
      std::string key = std::to_string(set.first);
      jSetHistogram[key] = set.second;
    }
    json jsonData;
    jsonData["Timer"] = jTimer;
    jsonData["Counter"] = jCounter;
    jsonData["Set Histogram"] = jSetHistogram;
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

  // for test purpose only
  void printTimerMap();
  void printStoppedTimer();
  void printCounterMap();
  void printSetHistoMap();
};

#endif /* PAMM_H_ */