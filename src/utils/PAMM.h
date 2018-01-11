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
using namespace std;
using json = nlohmann::json;
namespace bfs = boost::filesystem;

// The 'do {} while(0)' enabales to use the macros like a function
// since it does not anything, but can be followed by a semicolon.
#ifdef PERFORMANCE_EVA
#define START_TIMER(TIMER) p.startTimer(TIMER)
#define RESET_TIMER(TIMER) p.resetTimer(TIMER)
#define STOP_TIMER(TIMER) p.stopTimer(TIMER)
#define REG_COUNTER(COUNTER) p.regCounter(COUNTER)
#define INC_COUNTER(COUNTER) p.incCounter(COUNTER)
#define DEC_COUNTER(COUNTER) p.decCounter(COUNTER)
#define PRINT_EVA_RESULTS(CONFIG) p.printResults(CONFIG)
#define EXPORT_EVA_RESULTS(CONFIG) p.exportResultsAsJSON(CONFIG)
#else
#define START_TIMER(TIMERNAME)                                                 \
  do {                                                                         \
  } while (0)
#define RESET_TIMER(TIMERNAME)                                                 \
  do {                                                                         \
  } while (0)
#define STOP_TIMER(TIMERNAME)                                                  \
  do {                                                                         \
  } while (0)
#define REG_COUNTER(COUNTER)                                                   \
  do {                                                                         \
  } while (0)
#define INC_COUNTER(COUNTER)                                                   \
  do {                                                                         \
  } while (0)
#define DEC_COUNTER(COUNTER)                                                   \
  do {                                                                         \
  } while (0)
#define PRINT_EVA_RESULTS(CONFIG)                                              \
  do {                                                                         \
  } while (0)
#define EXPORT_EVA_RESULTS(CONFIG)                                             \
  do {                                                                         \
  } while (0)
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
  typedef chrono::high_resolution_clock::time_point time_point;
  map<const string, time_point> RunningTimer;
  map<const string, pair<time_point, time_point>> StoppedTimer;
  map<const string, unsigned> Counter;
  string getPrintableDuration(unsigned long duration);

public:
  PAMM(const PAMM &pm) = delete;
  PAMM(PAMM &&pm) = delete;
  PAMM &operator=(const PAMM &pm) = delete;
  PAMM &operator=(PAMM &&pm) = delete;
  static PAMM &getInstance();

  void startTimer(string timerId);
  void resetTimer(string timerId);
  void stopTimer(string timerId);

  void regCounter(string counterId);
  void incCounter(string counterId, unsigned value = 1);
  void decCounter(string counterId, unsigned value = 1);
  int counterValue(string counterId);

  void printTimerMap();
  void printStoppedTimer();
  void printCounterMap();

  template <typename Period = chrono::microseconds>
  unsigned long elapsedTime(string timerId) {
    auto timer = RunningTimer.find(timerId);
    if (timer != RunningTimer.end()) {
      time_point end = chrono::high_resolution_clock::now();
      time_point start = timer->second;
      auto duration = chrono::duration_cast<Period>(end - start);
      return duration.count();
    }
    auto result = StoppedTimer.find(timerId);
    if (result != StoppedTimer.end()) {
      auto duration = chrono::duration_cast<Period>(result->second.second -
                                                    result->second.first);
      return duration.count();
    } else {
      //      cout << timerId << " is not a valid timer!\n";
      return 0;
    }
  }

  template <typename Period = chrono::microseconds>
  unsigned long accumulatedTime(set<string> timers) {
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
  template <typename Period = chrono::microseconds>
  void printResults(const string &config) {
    // stop all running timer
    for (auto timer : RunningTimer) {
      stopTimer(timer.first);
    }
    cout << "\n----- EVALUATION RESULTS -----\n\n";
    cout << "Timer\n";
    cout << "-----\n";

    for (auto timer : StoppedTimer) {
      unsigned long time = elapsedTime<Period>(timer.first);
      cout << timer.first << " : " << getPrintableDuration(time) << '\n';
    }
    if (StoppedTimer.empty()) {
      cout << "No Timer started!" << '\n';
    }
    cout << "\nCounter\n";
    cout << "-------\n";
    for (auto counter : Counter) {
      cout << counter.first << " : " << counter.second << '\n';
    }
    if (Counter.empty()) {
      cout << "No Counter registered!" << '\n';
    }
  }

  /**
   * @brief Exports the measured results to JSON.
   * @tparam Period sets the precision for time computation.
   * @param config Config name of current analysis run.
   */
  template <typename Period = chrono::microseconds>
  void exportResultsAsJSON(const string &configPath) {
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
    string config = cfp.filename().string();
    size_t extensionPos = config.find(cfp.extension().string());
    config.replace(extensionPos, cfp.extension().size(), "");
    jsonResults["Config"] = config;
    jsonResults["Config path"] = configPath;
    ofstream file(config + ".json");
    file << setw(2) // sets the indentation
         << jsonResults << endl;
    file.close();
  }
};

#endif /* PAMM_H_ */