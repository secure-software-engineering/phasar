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

#include "PAMM.h"

PAMM &PAMM::getInstance() {
  static PAMM instance;
  return instance;
}

void PAMM::startTimer(std::string timerId) {
  bool validTimerId =
      !RunningTimer.count(timerId) && !StoppedTimer.count(timerId);
  assert(validTimerId && "startTimer failed due to an invalid timer id");
  if (validTimerId) {
    time_point start = std::chrono::high_resolution_clock::now();
    RunningTimer[timerId] = start;
  }
}

void PAMM::resetTimer(std::string timerId) {
  assert((RunningTimer.count(timerId) && !StoppedTimer.count(timerId)) ||
         (!RunningTimer.count(timerId) && StoppedTimer.count(timerId)) &&
             "resetTimer failed due to an invalid timer id");
  if (RunningTimer.count(timerId)) {
    RunningTimer.erase(RunningTimer.find(timerId));
  } else if (StoppedTimer.count(timerId)) {
    StoppedTimer.erase(StoppedTimer.find(timerId));
  }
}

void PAMM::stopTimer(std::string timerId, bool pauseTimer) {
  bool validTimerId =
      RunningTimer.count(timerId) && !StoppedTimer.count(timerId);
  assert(validTimerId && "stopTimer failed due to an invalid timer id or timer "
                         "was already stopped");
  if (validTimerId) {
    auto timer = RunningTimer.find(timerId);
    time_point end = std::chrono::high_resolution_clock::now();
    time_point start = timer->second;
    RunningTimer.erase(timer);
    auto p = make_pair(start, end);
    if (pauseTimer) {
      AccumulatedTimer[timerId].push_back(p);
    } else {
      StoppedTimer[timerId] = p;
    }
  }
}

std::string PAMM::getPrintableDuration(unsigned long duration) {
  unsigned long milliseconds = duration % 1000;
  unsigned long seconds = ((duration - milliseconds) / 1000) % 60;
  unsigned long minutes =
      ((((duration - milliseconds) / 1000) - seconds) / 60) % 60;
  unsigned long hours =
      (((((duration - milliseconds) / 1000) - seconds) / 60) - minutes) / 60;
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

void PAMM::regCounter(std::string counterId, unsigned intialValue) {
  bool validCounterId = !Counter.count(counterId);
  assert(validCounterId && "regCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[counterId] = intialValue;
  }
}

void PAMM::incCounter(std::string counterId, unsigned value) {
  bool validCounterId = Counter.count(counterId);
  assert(validCounterId && "incCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[counterId] += value;
  }
}

void PAMM::decCounter(std::string counterId, unsigned value) {
  bool validCounterId = Counter.count(counterId);
  assert(validCounterId && "decCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[counterId] -= value;
  }
}

int PAMM::getCounter(std::string counterId) {
  bool validCounterId = Counter.count(counterId);
  assert(validCounterId && "getCounter failed due to an invalid counter id");
  if (validCounterId) {
    return Counter[counterId];
  }
  return -1;
}

int PAMM::getSumCount(std::set<std::string> counterIds) {
  int sum = 0;
  for (std::string id : counterIds) {
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

void PAMM::regSetHistogram(std::string setHistoID) {
  bool validSetHistoID = !SetHistograms.count(setHistoID);
  assert(validSetHistoID && "failed to register new set histo due to an invalid id");
  if (validSetHistoID) {
    std::unordered_map<unsigned long, unsigned long> setH;
    SetHistograms[setHistoID] = setH;
  }
}

void PAMM::addToSetHistogram(std::string setHistoID, unsigned long setSize) {
  bool validSetHistoID = SetHistograms.count(setHistoID);
  assert(validSetHistoID && "adding set size to set histogram failed due to invalid id");
  if (SetHistograms[setHistoID].count(setSize)) {
    SetHistograms[setHistoID][setSize]++;
  } else {
    SetHistograms[setHistoID][setSize] = 1;
  }
}

void PAMM::printCounters() {
  std::cout << "\nCounter\n";
  std::cout << "-------\n";
  for (auto counter : Counter) {
    std::cout << counter.first << " : " << counter.second << '\n';
  }
  if (Counter.empty()) {
    std::cout << "No Counter registered!\n";
  } else {
    std::cout << "\n";
  }
}

void PAMM::printSetHistograms() {
  std::cout << "\nSet Histograms\n";
  std::cout << "--------------\n";
  for (auto setH : SetHistograms) {
    std::cout << setH.first << " Set Histogram\n";
    std::cout << "Size : Count\n";
    for (auto entry : setH.second) {
      std::cout << entry.first << " : " << entry.second << '\n';
    }
    std::cout << '\n';
  }
  if (SetHistograms.empty()) {
    std::cout << "No sets tracked!\n";
  }
}

void PAMM::reset() {
  RunningTimer.clear();
  StoppedTimer.clear();
  AccumulatedTimer.clear();
  Counter.clear();
  SetHistograms.clear();
  std::cout << "PAMM reseted!" << std::endl;
}

void PAMM::addSetHistogramToJSON(json &jsonData) {
  for (auto setH : SetHistograms) {
    json jSetH;
    for (auto entry : setH.second) {
      // json won't create a proper map when int/unsigned is used as a key
      jSetH[std::to_string(entry.first)] = entry.second;
    }
    jsonData[setH.first + " Set Histogram"] = jSetH;
  }
}

void PAMM::addCounterToJSON(json &jsonData) {
  json jFFCounter, jEFCounter, jGSCounter, jDFACounter, jCallsCounter,
      jGraphSizesCounter, jMiscCounter;
  for (auto counter : Counter) {
    if (counter.first.find("GS") != std::string::npos) {
      jGSCounter[counter.first] = counter.second;
    } else if (counter.first.find("Calls to") != std::string::npos) {
      jCallsCounter[counter.first] = counter.second;
    } else if (counter.first.find("-FF") != std::string::npos &&
               counter.first.find("Application") == std::string::npos) {
      jFFCounter[counter.first] = counter.second;
    } else if (counter.first.find("-EF") != std::string::npos) {
      jEFCounter[counter.first] = counter.second;
    } else if (counter.first.find("FF Construction") != std::string::npos ||
               counter.first.find("Propagation") != std::string::npos ||
               counter.first.find("FF Application") != std::string::npos) {
      jDFACounter[counter.first] = counter.second;
    } else if (counter.first.find("Edges") != std::string::npos ||
      counter.first.find("Vertices") != std::string::npos) {
      jGraphSizesCounter[counter.first] = counter.second;
    } else {
      jMiscCounter[counter.first] = counter.second;
    }
  }
  jsonData["Flow Function Counter"] = jFFCounter;
  jsonData["Edge Function Counter"] = jEFCounter;
  jsonData["DFA Counter"] = jDFACounter;
  jsonData["General Statistics"] = jGSCounter;
  jsonData["Function Call Counter"] = jCallsCounter;
  jsonData["Graph Sizes Counter"] = jGraphSizesCounter;
  if (!jMiscCounter.empty())
    jsonData["Misc Counter"] = jMiscCounter;
}

unsigned long PAMM::getSetHistoData(std::string setID, unsigned long setSize) {
  return SetHistograms[setID][setSize];
}
