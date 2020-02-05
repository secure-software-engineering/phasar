/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PAMM.cpp
 *
 *  Created on: 06.12.2017
 *      Author: rleer
 */

#include <cassert>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <phasar/Config/Configuration.h>
#include <phasar/Utils/PAMM.h>

using namespace psr;
using json = nlohmann::json;

namespace psr {

PAMM &PAMM::getInstance() {
  static PAMM instance;
  return instance;
}

void PAMM::startTimer(const std::string &TimerId) {
  bool validTimerId =
      !RunningTimer.count(TimerId) && !StoppedTimer.count(TimerId);
  assert(validTimerId && "startTimer failed due to an invalid timer id");
  if (validTimerId) {
    PAMM::TimePoint_t start = std::chrono::high_resolution_clock::now();
    RunningTimer[TimerId] = start;
  }
}

void PAMM::resetTimer(const std::string &TimerId) {
  assert((RunningTimer.count(TimerId) && !StoppedTimer.count(TimerId)) ||
         (!RunningTimer.count(TimerId) && StoppedTimer.count(TimerId)) &&
             "resetTimer failed due to an invalid timer id");
  if (RunningTimer.count(TimerId)) {
    RunningTimer.erase(RunningTimer.find(TimerId));
  } else if (StoppedTimer.count(TimerId)) {
    StoppedTimer.erase(StoppedTimer.find(TimerId));
  }
}

void PAMM::stopTimer(const std::string &TimerId, bool PauseTimer) {
  bool runningTimer = RunningTimer.count(TimerId);
  bool validTimerId = runningTimer || StoppedTimer.count(TimerId);
  assert(validTimerId && "stopTimer failed due to an invalid timer id or timer "
                         "was already stopped");
  assert(runningTimer && "stopTimer failed because timer was already stopped");
  if (validTimerId) {
    auto timer = RunningTimer.find(TimerId);
    PAMM::TimePoint_t end = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t start = timer->second;
    RunningTimer.erase(timer);
    auto p = make_pair(start, end);
    if (PauseTimer) {
      RepeatingTimer[TimerId].push_back(p);
    } else {
      StoppedTimer[TimerId] = p;
    }
  }
}

unsigned long PAMM::elapsedTime(const std::string &TimerId) {
  assert((RunningTimer.count(TimerId) || StoppedTimer.count(TimerId)) &&
         "elapsedTime failed due to an invalid timer id");
  if (RunningTimer.count(TimerId)) {
    PAMM::TimePoint_t end = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t start = RunningTimer[TimerId];
    auto duration = std::chrono::duration_cast<Duration_t>(end - start);
    return duration.count();
  } else if (StoppedTimer.count(TimerId)) {
    auto duration = std::chrono::duration_cast<Duration_t>(
        StoppedTimer[TimerId].second - StoppedTimer[TimerId].first);
    return duration.count();
  }
  return 0;
}

std::unordered_map<std::string, std::vector<unsigned long>>
PAMM::elapsedTimeOfRepeatingTimer() {
  std::unordered_map<std::string, std::vector<unsigned long>> accTimes;
  for (auto timer : RepeatingTimer) {
    std::vector<unsigned long> accTimeVec;
    for (auto timepair : timer.second) {
      auto duration = std::chrono::duration_cast<PAMM::Duration_t>(
          timepair.second - timepair.first);
      accTimeVec.push_back(duration.count());
    }
    accTimes[timer.first] = accTimeVec;
  }
  return accTimes;
}

std::string PAMM::getPrintableDuration(unsigned long Duration) {
  unsigned long milliseconds = Duration % 1000;
  unsigned long seconds = ((Duration - milliseconds) / 1000) % 60;
  unsigned long minutes =
      ((((Duration - milliseconds) / 1000) - seconds) / 60) % 60;
  unsigned long hours =
      (((((Duration - milliseconds) / 1000) - seconds) / 60) - minutes) / 60;
  std::ostringstream oss;
  if (hours)
    oss << hours << "h ";
  if (minutes)
    oss << minutes << "m ";
  if (seconds)
    oss << seconds << "sec ";
  if (milliseconds)
    oss << milliseconds << "ms";
  else
    oss << "0 ms";
  return oss.str();
}

void PAMM::regCounter(const std::string &CounterId, unsigned IntialValue) {
  bool validCounterId = !Counter.count(CounterId);
  assert(validCounterId && "regCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[CounterId] = IntialValue;
  }
}

void PAMM::incCounter(const std::string &CounterId, unsigned CValue) {
  bool validCounterId = Counter.count(CounterId);
  assert(validCounterId && "incCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[CounterId] += CValue;
  }
}

void PAMM::decCounter(const std::string &CounterId, unsigned CValue) {
  bool validCounterId = Counter.count(CounterId);
  assert(validCounterId && "decCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[CounterId] -= CValue;
  }
}

int PAMM::getCounter(const std::string &CounterId) {
  bool validCounterId = Counter.count(CounterId);
  assert(validCounterId && "getCounter failed due to an invalid counter id");
  if (validCounterId) {
    return Counter[CounterId];
  }
  return -1;
}

int PAMM::getSumCount(const std::set<std::string> &CounterIds) {
  int sum = 0;
  for (std::string id : CounterIds) {
    int count = getCounter(id);
    assert(count != -1 && "getSumCount failed due to an invalid counter id");
    if (count != -1) {
      sum += count;
    } else {
      return -1;
    }
  }
  return sum;
}

void PAMM::regHistogram(const std::string &HistogramId) {
  bool validHID = !Histogram.count(HistogramId);
  assert(validHID && "failed to register new histogram due to an invalid id");
  if (validHID) {
    std::unordered_map<std::string, unsigned long> H;
    Histogram[HistogramId] = H;
  }
}

void PAMM::addToHistogram(const std::string &HistogramId,
                          const std::string &DataPointId,
                          unsigned long DataPointValue) {
  bool validHistoID = Histogram.count(HistogramId);
  assert(validHistoID &&
         "adding data point to histogram failed due to invalid id");
  if (Histogram[HistogramId].count(DataPointId)) {
    Histogram[HistogramId][DataPointId] += DataPointValue;
  } else {
    Histogram[HistogramId][DataPointId] = DataPointValue;
  }
}

void PAMM::printTimers(std::ostream &os) {
  // stop all running timer
  while (!RunningTimer.empty()) {
    stopTimer(RunningTimer.begin()->first);
  }
  os << "Single Timer\n";
  os << "------------\n";
  for (auto timer : StoppedTimer) {
    unsigned long time = elapsedTime(timer.first);
    os << timer.first << " : " << getPrintableDuration(time) << '\n';
  }
  if (StoppedTimer.empty()) {
    os << "No single Timer started!\n\n";
  } else {
    os << "\n";
  }
  os << "Repeating Timer\n";
  os << "---------------\n";
  for (auto timer : elapsedTimeOfRepeatingTimer()) {
    unsigned long sum = 0;
    os << timer.first << " Timer:\n";
    for (auto duration : timer.second) {
      sum += duration;
      os << duration << '\n';
    }
    os << "===\n" << sum << "\n\n";
  }
  if (RepeatingTimer.empty()) {
    os << "No repeating Timer found!\n";
  } else {
    os << '\n';
  }
}

void PAMM::printCounters(std::ostream &os) {
  os << "\nCounter\n";
  os << "-------\n";
  for (auto counter : Counter) {
    os << counter.first << " : " << counter.second << '\n';
  }
  if (Counter.empty()) {
    os << "No Counter registered!\n";
  } else {
    os << "\n";
  }
}

void PAMM::printHistograms(std::ostream &os) {
  os << "\nHistograms\n";
  os << "--------------\n";
  for (auto H : Histogram) {
    os << H.first << " Histogram\n";
    os << "Value : #Occurrences\n";
    for (auto entry : H.second) {
      os << entry.first << " : " << entry.second << '\n';
    }
    os << '\n';
  }
  if (Histogram.empty()) {
    os << "No histograms tracked!\n";
  }
}

void PAMM::printMeasuredData(std::ostream &os) {
  os << "\n----- START OF EVALUATION DATA -----\n\n";
  printTimers(os);
  printCounters(os);
  printHistograms(os);
  os << "\n----- END OF EVALUATION DATA -----\n\n";
}

void PAMM::exportMeasuredData(std::string OutputPath) {
  // json file for holding all data
  json jsonData;

  // add timer data
  while (!RunningTimer.empty()) {
    stopTimer(std::string(RunningTimer.begin()->first));
  }
  json jTimer;
  for (auto timer : StoppedTimer) {
    unsigned long time = elapsedTime(timer.first);
    jTimer[timer.first] = time;
  }
  for (auto timer : elapsedTimeOfRepeatingTimer()) {
    jTimer[timer.first] = timer.second;
  }
  jsonData["Timer"] = jTimer;

  // add histogram data if available
  json jHistogram;
  for (auto H : Histogram) {
    json jSetH;
    for (auto entry : H.second) {
      jSetH[entry.first] = entry.second;
    }
    jHistogram[H.first] = jSetH;
  }
  if (!jHistogram.is_null()) {
    jsonData["Histogram"] = jHistogram;
  }
  // add counter data
  json jCounter;
  for (auto counter : Counter) {
    jCounter[counter.first] = counter.second;
  }
  jsonData["Counter"] = jCounter;

  // add analysis/project/source file information if available
  json jInfo;
  if (PhasarConfig::VariablesMap().count("project-id")) {
    jInfo["Project-ID"] =
        PhasarConfig::VariablesMap()["project-id"].as<std::string>();
  }
  if (PhasarConfig::VariablesMap().count("module")) {
    jInfo["Module(s)"] =
        PhasarConfig::VariablesMap()["module"].as<std::vector<std::string>>();
  }
  if (PhasarConfig::VariablesMap().count("data-flow-analysis")) {
    jInfo["Data-flow analysis"] =
        PhasarConfig::VariablesMap()["data-flow-analysis"]
            .as<std::vector<std::string>>();
  }
  if (!jInfo.is_null()) {
    jsonData["Info"] = jInfo;
  }

  boost::filesystem::path cfp(OutputPath);
  if (cfp.string().find(".json") == std::string::npos) {
    OutputPath.append(".json");
  }
  std::ofstream file(OutputPath);
  if (file.is_open()) {
    file << std::setw(2) // sets the indentation
         << jsonData << std::endl;
    file.close();
  } else {
    throw std::ios_base::failure("could not write file: " + OutputPath);
  }
}

void PAMM::reset() {
  RunningTimer.clear();
  StoppedTimer.clear();
  RepeatingTimer.clear();
  Counter.clear();
  Histogram.clear();
}
} // namespace psr
