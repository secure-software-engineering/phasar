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
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

// for convenience
using json = nlohmann::json;
namespace bfs = boost::filesystem;

// The 'do {} while(0)' enabales to use the macros like a function
// since it does not anything, but can be followed by a semicolon.
#ifdef PERFORMANCE_EVA
#define PAMM_FACTORY PAMM &p = PAMM::getInstance()
#define START_TIMER(TIMER) p.startTimer(TIMER)
#define RESET_TIMER(TIMER) p.resetTimer(TIMER)
#define STOP_TIMER(TIMER) p.stopTimer(TIMER)
#define REG_COUNTER(COUNTER) p.regCounter(COUNTER)
#define INC_COUNTER(COUNTER) p.incCounter(COUNTER)
#define DEC_COUNTER(COUNTER) p.decCounter(COUNTER)
#define PRINT_EVA_RESULTS(CONFIG) p.printResults(CONFIG)
#define EXPORT_EVA_RESULTS(CONFIG) p.exportResultsAsJSON(CONFIG)
#else
#define PAMM_FACTORY
#define START_TIMER(TIMER)
#define RESET_TIMER(TIMER)
#define STOP_TIMER(TIMER)
#define REG_COUNTER(COUNTER)
#define INC_COUNTER(COUNTER)
#define DEC_COUNTER(COUNTER)
#define PRINT_EVA_RESULTS(CONFIG)
#define EXPORT_EVA_RESULTS(CONFIG)
#endif

/**
 * This class offers functionality to measure different performance metrics.
 * All functions are wrapped into preprocessor macros.
 * @brief This class offers functionality to perform a performance analysis.
 * @note This class implements the Singleton Pattern - use the getInstance()
 * factory method to receive a reference to the PAMM object.
 */
class PAMM {
private:
  PAMM() = default;
  ~PAMM() = default;
  typedef std::chrono::high_resolution_clock::time_point time_point;
  std::map<const std::string, time_point> RunningTimer;
  std::map<const std::string, std::pair<time_point, time_point>> StoppedTimer;
  std::map<const std::string, unsigned> Counter;
  std::string getPrintableDuration(unsigned long duration);

public:
  PAMM(const PAMM &pm) = delete;
  PAMM(PAMM &&pm) = delete;
  PAMM &operator=(const PAMM &pm) = delete;
  PAMM &operator=(PAMM &&pm) = delete;
  static PAMM &getInstance();

  void startTimer(std::string timerId);
  void resetTimer(std::string timerId);
  void stopTimer(std::string timerId);

  void regCounter(std::string counterId);
  void incCounter(std::string counterId, unsigned value = 1);
  void decCounter(std::string counterId, unsigned value = 1);
  int counterValue(std::string counterId);

  void printTimerMap();
  void printStoppedTimer();
  void printCounterMap();

  template <typename Period = std::chrono::microseconds>
  unsigned long elapsedTime(std::string timerId) {
    auto timer = RunningTimer.find(timerId);
    if (timer != RunningTimer.end()) {
      time_point end = std::chrono::high_resolution_clock::now();
      time_point start = timer->second;
      auto duration = std::chrono::duration_cast<Period>(end - start);
      return duration.count();
    }
    auto result = StoppedTimer.find(timerId);
    if (result != StoppedTimer.end()) {
      auto duration = std::chrono::duration_cast<Period>(result->second.second -
                                                    result->second.first);
      return duration.count();
    } else {
      //      cout << timerId << " is not a valid timer!\n";
      return 0;
    }
  }

  template <typename Period = std::chrono::microseconds>
  unsigned long accumulatedTime(std::set<std::string> timers) {
    unsigned long accTime = 0;
    for (auto timerId : timers) {
      accTime += elapsedTime<Period>(timerId);
    }
    return accTime;
  }

  /**
   * @brief Prints the measured results to the command line.
   * @tparam Period sets the precision for time computation.
   * @param config Config name of current analysis run.
   */
  template <typename Period = std::chrono::microseconds>
  void printResults(const std::string &config) {
    // stop all running timer
    for (auto timer : RunningTimer) {
      stopTimer(timer.first);
    }
    std::cout << "\n----- EVALUATION RESULTS -----\n\n";
    std::cout << "Timer\n";
    std::cout << "-----\n";

    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      std::cout << timer.first << " : " << getPrintableDuration(time) << '\n';
    }
    if (StoppedTimer.empty()) {
      std::cout << "No Timer started!" << '\n';
    }
    std::cout << "\nCounter\n";
    std::cout << "-------\n";
    for (auto counter : Counter) {
      std::cout << counter.first << " : " << counter.second << '\n';
    }
    if (Counter.empty()) {
      std::cout << "No Counter registered!" << '\n';
    }
  }

  /**
   * @brief Exports the measured results to JSON.
   * @tparam Period sets the precision for time computation.
   * @param config Config name of current analysis run.
   */
  template <typename Period = std::chrono::microseconds>
  void exportResultsAsJSON(const std::string &configPath) {
    // stop all running timer
    for (auto timer : RunningTimer) {
      stopTimer(timer.first);
    }
    json jTimer;
    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      // TODO use a better format than just a string for the elapsed time
      jTimer[timer.first] = getPrintableDuration(time);
    }
    json jCounter;
    for (auto counter : Counter) {
      jCounter[counter.first] = counter.second;
    }
    json jsonResults;
    jsonResults["Timer"] = jTimer;
    jsonResults["Counter"] = jCounter;
    bfs::path cfp(configPath);
    // reduce the config path to just the filename - no path and no extension
    std::string config = cfp.filename().string();
    std::size_t extensionPos = config.find(cfp.extension().string());
    config.replace(extensionPos, cfp.extension().size(), "");
    jsonResults["Config"] = config;
    jsonResults["Config path"] = configPath;
    std::ofstream file(config + ".json");
    file << std::setw(2) // sets the indentation
         << jsonResults << std::endl;
    file.close();
  }
};

#endif /* PAMM_H_ */