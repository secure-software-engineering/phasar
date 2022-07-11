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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/PAMM.h"

using namespace psr;
using json = nlohmann::json;

namespace psr {

PAMM &PAMM::getInstance() {
  static PAMM Instance;
  return Instance;
}

void PAMM::startTimer(const std::string &TimerId) {
  bool ValidTimerId =
      !RunningTimer.count(TimerId) && !StoppedTimer.count(TimerId);
  assert(ValidTimerId && "startTimer failed due to an invalid timer id");
  if (ValidTimerId) {
    PAMM::TimePoint_t Start = std::chrono::high_resolution_clock::now();
    RunningTimer[TimerId] = Start;
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
  bool TimerRunning = RunningTimer.count(TimerId);
  bool ValidTimerId = TimerRunning || StoppedTimer.count(TimerId);
  assert(ValidTimerId && "stopTimer failed due to an invalid timer id or timer "
                         "was already stopped");
  assert(TimerRunning && "stopTimer failed because timer was already stopped");
  if (ValidTimerId) {
    auto Timer = RunningTimer.find(TimerId);
    PAMM::TimePoint_t End = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t Start = Timer->second;
    RunningTimer.erase(Timer);
    auto P = make_pair(Start, End);
    if (PauseTimer) {
      RepeatingTimer[TimerId].push_back(P);
    } else {
      StoppedTimer[TimerId] = P;
    }
  }
}

unsigned long PAMM::elapsedTime(const std::string &TimerId) {
  assert((RunningTimer.count(TimerId) || StoppedTimer.count(TimerId)) &&
         "elapsedTime failed due to an invalid timer id");
  if (RunningTimer.count(TimerId)) {
    PAMM::TimePoint_t End = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t Start = RunningTimer[TimerId];
    auto Duration = std::chrono::duration_cast<Duration_t>(End - Start);
    return Duration.count();
  }
  if (StoppedTimer.count(TimerId)) {
    auto Duration = std::chrono::duration_cast<Duration_t>(
        StoppedTimer[TimerId].second - StoppedTimer[TimerId].first);
    return Duration.count();
  }
  return 0;
}

std::unordered_map<std::string, std::vector<unsigned long>>
PAMM::elapsedTimeOfRepeatingTimer() {
  std::unordered_map<std::string, std::vector<unsigned long>> AccTimes;
  for (const auto &Timer : RepeatingTimer) {
    std::vector<unsigned long> AccTimeVec;
    for (auto Timepair : Timer.second) {
      auto Duration = std::chrono::duration_cast<PAMM::Duration_t>(
          Timepair.second - Timepair.first);
      AccTimeVec.push_back(Duration.count());
    }
    AccTimes[Timer.first] = AccTimeVec;
  }
  return AccTimes;
}

std::string PAMM::getPrintableDuration(unsigned long Duration) {
  unsigned long Milliseconds = Duration % 1000;
  unsigned long Seconds = ((Duration - Milliseconds) / 1000) % 60;
  unsigned long Minutes =
      ((((Duration - Milliseconds) / 1000) - Seconds) / 60) % 60;
  unsigned long Hours =
      (((((Duration - Milliseconds) / 1000) - Seconds) / 60) - Minutes) / 60;
  std::ostringstream Oss;
  if (Hours) {
    Oss << Hours << "h ";
  }
  if (Minutes) {
    Oss << Minutes << "m ";
  }
  if (Seconds) {
    Oss << Seconds << "sec ";
  }
  if (Milliseconds) {
    Oss << Milliseconds << "ms";
  } else {
    Oss << "0 ms";
  }
  return Oss.str();
}

void PAMM::regCounter(const std::string &CounterId, unsigned IntialValue) {
  bool ValidCounterId = !Counter.count(CounterId);
  assert(ValidCounterId && "regCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    Counter[CounterId] = IntialValue;
  }
}

void PAMM::incCounter(const std::string &CounterId, unsigned CValue) {
  bool ValidCounterId = Counter.count(CounterId);
  assert(ValidCounterId && "incCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    Counter[CounterId] += CValue;
  }
}

void PAMM::decCounter(const std::string &CounterId, unsigned CValue) {
  bool ValidCounterId = Counter.count(CounterId);
  assert(ValidCounterId && "decCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    Counter[CounterId] -= CValue;
  }
}

int PAMM::getCounter(const std::string &CounterId) {
  bool ValidCounterId = Counter.count(CounterId);
  assert(ValidCounterId && "getCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    return Counter[CounterId];
  }
  return -1;
}

int PAMM::getSumCount(const std::set<std::string> &CounterIds) {
  int Sum = 0;
  for (const std::string &Id : CounterIds) {
    int Count = getCounter(Id);
    assert(Count != -1 && "getSumCount failed due to an invalid counter id");
    if (Count != -1) {
      Sum += Count;
    } else {
      return -1;
    }
  }
  return Sum;
}

void PAMM::regHistogram(const std::string &HistogramId) {
  bool ValidHid = !Histogram.count(HistogramId);
  assert(ValidHid && "failed to register new histogram due to an invalid id");
  if (ValidHid) {
    std::unordered_map<std::string, unsigned long> H;
    Histogram[HistogramId] = H;
  }
}

void PAMM::addToHistogram(const std::string &HistogramId,
                          const std::string &DataPointId,
                          unsigned long DataPointValue) {
  assert(Histogram.count(HistogramId) &&
         "adding data point to histogram failed due to invalid id");
  if (Histogram[HistogramId].count(DataPointId)) {
    Histogram[HistogramId][DataPointId] += DataPointValue;
  } else {
    Histogram[HistogramId][DataPointId] = DataPointValue;
  }
}

void PAMM::printTimers(llvm::raw_ostream &Os) {
  // stop all running timer
  while (!RunningTimer.empty()) {
    stopTimer(RunningTimer.begin()->first);
  }
  Os << "Single Timer\n";
  Os << "------------\n";
  for (const auto &Timer : StoppedTimer) {
    unsigned long Time = elapsedTime(Timer.first);
    Os << Timer.first << " : " << getPrintableDuration(Time) << '\n';
  }
  if (StoppedTimer.empty()) {
    Os << "No single Timer started!\n\n";
  } else {
    Os << "\n";
  }
  Os << "Repeating Timer\n";
  Os << "---------------\n";
  for (const auto &Timer : elapsedTimeOfRepeatingTimer()) {
    unsigned long Sum = 0;
    Os << Timer.first << " Timer:\n";
    for (auto Duration : Timer.second) {
      Sum += Duration;
      Os << Duration << '\n';
    }
    Os << "===\n" << Sum << "\n\n";
  }
  if (RepeatingTimer.empty()) {
    Os << "No repeating Timer found!\n";
  } else {
    Os << '\n';
  }
}

void PAMM::printCounters(llvm::raw_ostream &Os) {
  Os << "\nCounter\n";
  Os << "-------\n";
  for (const auto &Counter : Counter) {
    Os << Counter.first << " : " << Counter.second << '\n';
  }
  if (Counter.empty()) {
    Os << "No Counter registered!\n";
  } else {
    Os << "\n";
  }
}

void PAMM::printHistograms(llvm::raw_ostream &Os) {
  Os << "\nHistograms\n";
  Os << "--------------\n";
  for (const auto &H : Histogram) {
    Os << H.first << " Histogram\n";
    Os << "Value : #Occurrences\n";
    for (const auto &Entry : H.second) {
      Os << Entry.first << " : " << Entry.second << '\n';
    }
    Os << '\n';
  }
  if (Histogram.empty()) {
    Os << "No histograms tracked!\n";
  }
}

void PAMM::printMeasuredData(llvm::raw_ostream &Os) {
  Os << "\n----- START OF EVALUATION DATA -----\n\n";
  printTimers(Os);
  printCounters(Os);
  printHistograms(Os);
  Os << "\n----- END OF EVALUATION DATA -----\n\n";
}

void PAMM::exportMeasuredData(std::string OutputPath) {
  // json file for holding all data
  json JsonData;

  // add timer data
  while (!RunningTimer.empty()) {
    stopTimer(std::string(RunningTimer.begin()->first));
  }
  json JTimer;
  for (const auto &Timer : StoppedTimer) {
    unsigned long Time = elapsedTime(Timer.first);
    JTimer[Timer.first] = Time;
  }
  for (const auto &Timer : elapsedTimeOfRepeatingTimer()) {
    JTimer[Timer.first] = Timer.second;
  }
  JsonData["Timer"] = JTimer;

  // add histogram data if available
  json JHistogram;
  for (const auto &H : Histogram) {
    json JSetH;
    for (const auto &Entry : H.second) {
      JSetH[Entry.first] = Entry.second;
    }
    JHistogram[H.first] = JSetH;
  }
  if (!JHistogram.is_null()) {
    JsonData["Histogram"] = JHistogram;
  }
  // add counter data
  json JCounter;
  for (const auto &Counter : Counter) {
    JCounter[Counter.first] = Counter.second;
  }
  JsonData["Counter"] = JCounter;

  // add analysis/project/source file information if available
  json JInfo;
  if (PhasarConfig::VariablesMap().count("project-id")) {
    JInfo["Project-ID"] =
        PhasarConfig::VariablesMap()["project-id"].as<std::string>();
  }
  if (PhasarConfig::VariablesMap().count("module")) {
    JInfo["Module(s)"] =
        PhasarConfig::VariablesMap()["module"].as<std::vector<std::string>>();
  }
  if (PhasarConfig::VariablesMap().count("data-flow-analysis")) {
    JInfo["Data-flow analysis"] =
        PhasarConfig::VariablesMap()["data-flow-analysis"]
            .as<std::vector<std::string>>();
  }
  if (!JInfo.is_null()) {
    JsonData["Info"] = JInfo;
  }

  std::filesystem::path Cfp(OutputPath);
  if (Cfp.string().find(".json") == std::string::npos) {
    OutputPath.append(".json");
  }
  std::ofstream File(OutputPath);
  if (File.is_open()) {
    File << std::setw(2) // sets the indentation
         << JsonData << std::endl;
    File.close();
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
