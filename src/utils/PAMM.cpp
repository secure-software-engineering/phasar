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
  if (RunningTimer.find(timerId) == RunningTimer.end() &&
      StoppedTimer.find(timerId) == StoppedTimer.end()) {
    time_point start = std::chrono::high_resolution_clock::now();
    RunningTimer[timerId] = start;
    std::cout << timerId << " started.\n";
  } else {
    std::cout << timerId << " already started or stopped!\n";
  }
}

void PAMM::resetTimer(std::string timerId) {
  auto timer = RunningTimer.find(timerId);
  if (timer != RunningTimer.end()) {
    RunningTimer.erase(timer);
  }
  auto result = StoppedTimer.find(timerId);
  if (result != StoppedTimer.end()) {
    StoppedTimer.erase(result);
  } else {
    std::cout << timerId << " is not a valid timer!\n";
  }
}

void PAMM::stopTimer(std::string timerId) {
  auto timer = RunningTimer.find(timerId);
  if (timer != RunningTimer.end()) {
    time_point end = std::chrono::high_resolution_clock::now();
    time_point start = timer->second;
    RunningTimer.erase(timer);
    //      pair<time_point, time_point> p(start, end);
    auto p = make_pair(start, end);
    StoppedTimer[timerId] = p;
    //    std::cout << timerId << " stopped!\n";
  } else {
    std::cout << timerId << " is not a valid timer!\n";
  }
}

void PAMM::regCounter(std::string counterId) {
  auto counter = Counter.find(counterId);
  if (counter == Counter.end()) {
    Counter[counterId] = 0;
    std::cout << "counter " << counterId << " registered.\n";
  } else {
    std::cout << counterId << " already exists!\n";
  }
}

void PAMM::incCounter(std::string counterId, unsigned value) {
  auto counter = Counter.find(counterId);
  if (counter != Counter.end() && (counter->second + value > counter->second)) {
    counter->second += value;
  }
}

void PAMM::decCounter(std::string counterId, unsigned value) {
  auto counter = Counter.find(counterId);
  if (counter != Counter.end() && (counter->second - value < counter->second)) {
    counter->second -= value;
  }
}

int PAMM::getCounter(std::string counterId) {
  auto countIt = Counter.find(counterId);
  if (countIt != Counter.end()) {
    return countIt->second;
  } else {
    // counter not found
    return -1;
  }
}

std::string PAMM::getPrintableDuration(unsigned long duration) {
  unsigned long milliseconds = (unsigned long)(duration / 1000) % 1000;
  unsigned long seconds =
      (((unsigned long)(duration / 1000) - milliseconds) / 1000) % 60;
  unsigned long minutes =
      (((((unsigned long)(duration / 1000) - milliseconds) / 1000) - seconds) /
       60) %
      60;
  unsigned long hours =
      ((((((unsigned long)(duration / 1000) - milliseconds) / 1000) - seconds) /
        60) -
       minutes) /
      60;
  std::ostringstream oss;
  if (hours)
    oss << hours << "hr ";
  if (minutes)
    oss << minutes << "m ";
  if (seconds)
    oss << seconds << "sec ";
  if (milliseconds)
    oss << milliseconds << "ms";
  else
    oss << "0 ms";
  //  oss << setfill('0') // set field fill character to '0'
  //      << hours << "hr " << setw(2) << minutes << "m " << setw(2) << seconds
  //      << "sec " << setw(3) << milliseconds << "ms";
  return oss.str();
}

void PAMM::printTimerMap() {
  std::cout << "Running timer: [ ";
  for (auto entry : RunningTimer) {
    std::cout << entry.first << ":" << getPrintableDuration(elapsedTime(entry.first))
         << " ";
  }
  std::cout << "]\n";
}

void PAMM::printStoppedTimer() {
  std::cout << "Stopped timer: [ ";
  for (auto entry : StoppedTimer) {
    std::cout << entry.first << ":" << getPrintableDuration(elapsedTime(entry.first))
         << " ";
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
