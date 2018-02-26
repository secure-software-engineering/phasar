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

#include <phasar/Utils/PAMM.h>

PAMM &PAMM::getInstance() {
  static PAMM instance;
  return instance;
}
// TODO: check why assert does not get triggered (unlike me right now)
void PAMM::startTimer(std::string timerId) {
  bool validTimerId =
    !RunningTimer.count(timerId) && !StoppedTimer.count(timerId);
  assert(validTimerId && "startTimer failed due to an invalid timer id");
  if (validTimerId) {
    time_point start = std::chrono::high_resolution_clock::now();
    RunningTimer[timerId] = start;
    std::cout << timerId << " started.\n";
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
  std::cout << timerId << " reseted.\n";
}

void PAMM::stopTimer(std::string timerId) {
  bool validTimerId =
      RunningTimer.count(timerId) && !StoppedTimer.count(timerId);
  assert(validTimerId && "stopTimer failed due to an invalid timer id or timer was already stopped");
  if (validTimerId) {
    auto timer = RunningTimer.find(timerId);
    time_point end = std::chrono::high_resolution_clock::now();
    time_point start = timer->second;
    RunningTimer.erase(timer);
    auto p = make_pair(start, end);
    StoppedTimer[timerId] = p;
    std::cout << timerId << " stopped.\n";
  }
}

std::string PAMM::getPrintableDuration(unsigned long duration) {
  unsigned long milliseconds = duration % 1000;
  unsigned long seconds = ((duration - milliseconds) / 1000) % 60;
  unsigned long minutes =
      ((((duration - milliseconds) / 1000) - seconds) / 60) % 60;
  unsigned long hours =
      (((((duration - milliseconds) / 1000) - seconds) / 60) -
       minutes) / 60;
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

void PAMM::regCounter(std::string counterId) {
  bool validCounterId = !Counter.count(counterId);
  assert(validCounterId && "regCounter failed due to an invalid counter id");
  if (validCounterId) {
    Counter[counterId] = 0;
    std::cout << "counter " << counterId << " registered.\n";
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

void PAMM::addDataToSetHistogram(unsigned long setSize) {
  if (SetHistogram.count(setSize)) {
    SetHistogram[setSize]++;
  } else {
    SetHistogram[setSize] = 1;
  }
}

unsigned long PAMM::getSetHistoData(unsigned long setSize) {
  if (SetHistogram.count(setSize)) {
    return SetHistogram[setSize];
  }
  return 0;
}

void PAMM::printTimerMap() {
  std::cout << "Running timer: [ ";
  for (auto entry : RunningTimer) {
    std::cout << entry.first << ":"
              << getPrintableDuration(elapsedTime(entry.first)) << " ";
  }
  std::cout << "]\n";
}

void PAMM::printStoppedTimer() {
  std::cout << "Stopped timer: [ ";
  for (auto entry : StoppedTimer) {
    std::cout << entry.first << ":"
              << getPrintableDuration(elapsedTime(entry.first)) << " ";
  }
  std::cout << "]\n";
}

void PAMM::printCounterMap() {
  std::cout << "Counter: [ ";
  for (auto counter : Counter) {
    std::cout << counter.first << "=" << counter.second << " ";
  }
  std::cout << "]\n";
}

void PAMM::printSetHistoMap() {
  std::cout << "Set Histogram Data:\n";
  for (auto hg : SetHistogram) {
    std::cout << hg.first << ": [ " << hg.second << " ]\n";
  }
}

void PAMM::reset() {
  RunningTimer.clear();
  StoppedTimer.clear();
  Counter.clear();
  SetHistogram.clear();
  std::cout << "PAMM reseted!" << std::endl;
}
