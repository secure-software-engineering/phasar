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

#include "phasar/Utils/PAMM.h"

#include "phasar/Utils/ChronoUtils.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <system_error>

using namespace psr;
using json = nlohmann::json;

namespace psr {

PAMM &PAMM::getInstance() {
  static PAMM Instance{};
  return Instance;
}

void PAMM::startTimer(llvm::StringRef TimerId) {
  if (LLVM_UNLIKELY(StoppedTimer.count(TimerId))) {
    llvm::report_fatal_error("Do not start an already stopped timer");
  }

  auto [It, Inserted] = RunningTimer.try_emplace(TimerId);
  if (LLVM_UNLIKELY(!Inserted)) {
    llvm::report_fatal_error("Do not start an already running timer");
  }

  PAMM::TimePoint_t Start = std::chrono::high_resolution_clock::now();
  It->second = Start;
}

void PAMM::resetTimer(llvm::StringRef TimerId) {
  bool InRunningTimers = RunningTimer.erase(TimerId);
  bool InStoppedTimers = StoppedTimer.erase(TimerId);

  assert((InRunningTimers && !InStoppedTimers) ||
         (!InRunningTimers && InStoppedTimers) &&
             "resetTimer failed due to an invalid timer id");
}

void PAMM::stopTimer(llvm::StringRef TimerId, bool PauseTimer) {
  auto RunningIt = RunningTimer.find(TimerId);
  auto StoppedIt = StoppedTimer.find(TimerId);
  bool TimerRunning = RunningIt != RunningTimer.end();
  bool TimerStopped = StoppedIt != StoppedTimer.end();
  bool ValidTimerId = TimerRunning || TimerStopped;
  assert(ValidTimerId && "stopTimer failed due to an invalid timer id or timer "
                         "was already stopped");
  assert(TimerRunning && "stopTimer failed because timer was already stopped");

  if (LLVM_LIKELY(ValidTimerId)) {
    PAMM::TimePoint_t End = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t Start = RunningIt->second;
    RunningTimer.erase(RunningIt);
    auto P = make_pair(Start, End);
    if (PauseTimer) {
      RepeatingTimer[TimerId].push_back(P);
    } else {
      StoppedTimer[TimerId] = P;
    }
  }
}

uint64_t PAMM::elapsedTime(llvm::StringRef TimerId) {
  auto RunningIt = RunningTimer.find(TimerId);

  if (RunningIt != RunningTimer.end()) {
    PAMM::TimePoint_t End = std::chrono::high_resolution_clock::now();
    PAMM::TimePoint_t Start = RunningIt->second;
    auto Duration = std::chrono::duration_cast<Duration_t>(End - Start);
    return Duration.count();
  }
  if (auto StoppedIt = StoppedTimer.find(TimerId);
      StoppedIt != StoppedTimer.end()) {
    auto [Start, End] = StoppedIt->second;
    auto Duration = std::chrono::duration_cast<Duration_t>(End - Start);
    return Duration.count();
  }

  assert(false && "elapsedTime failed due to an invalid timer id");
  return 0;
}

template <typename HandlerFn>
static void foreachElapsedTimeOfRepeatingTimer(
    llvm::StringMap<
        std::vector<std::pair<PAMM::TimePoint_t, PAMM::TimePoint_t>>>
        &RepeatingTimer,
    HandlerFn Handler) {
  for (const auto &Timer : RepeatingTimer) {
    std::invoke(
        Handler, Timer.first(), [&Timer](std::vector<uint64_t> &AccTimeVec) {
          AccTimeVec.reserve(Timer.second.size());

          for (auto [Start, End] : Timer.second) {
            auto Duration =
                std::chrono::duration_cast<PAMM::Duration_t>(End - Start);
            AccTimeVec.push_back(Duration.count());
          }
        });
  }
}

llvm::StringMap<std::vector<uint64_t>> PAMM::elapsedTimeOfRepeatingTimer() {
  llvm::StringMap<std::vector<uint64_t>> AccTimes;

  foreachElapsedTimeOfRepeatingTimer(
      RepeatingTimer, [&AccTimes](llvm::StringRef Id, auto Handler) {
        std::invoke(std::move(Handler), AccTimes[Id]);
      });

  return AccTimes;
}

std::string PAMM::getPrintableDuration(uint64_t Duration) {
  return hms(std::chrono::milliseconds{Duration}).str();
}

void PAMM::regCounter(llvm::StringRef CounterId, unsigned IntialValue) {
  auto [It, Inserted] = Counter.try_emplace(CounterId, IntialValue);
  assert(Inserted && "regCounter failed due to an invalid counter id");
}

void PAMM::incCounter(llvm::StringRef CounterId, unsigned CValue) {
  auto It = Counter.find(CounterId);
  bool ValidCounterId = It != Counter.end();
  assert(ValidCounterId && "incCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    It->second += CValue;
  }
}

void PAMM::decCounter(llvm::StringRef CounterId, unsigned CValue) {
  auto It = Counter.find(CounterId);
  bool ValidCounterId = It != Counter.end();
  assert(ValidCounterId && "decCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    It->second -= CValue;
  }
}

std::optional<unsigned> PAMM::getCounter(llvm::StringRef CounterId) {
  auto It = Counter.find(CounterId);
  bool ValidCounterId = It != Counter.end();
  assert(ValidCounterId && "getCounter failed due to an invalid counter id");
  if (ValidCounterId) {
    return It->second;
  }
  return std::nullopt;
}

template <typename ForwardIterator, typename ForwardIteratorSentinel>
static std::optional<uint64_t>
getSumCountInternal(PAMM &P, ForwardIterator It,
                    ForwardIteratorSentinel End) noexcept {
  uint64_t Ctr = 0;
  for (; It != End; ++It) {
    auto Count = P.getCounter(*It);
    if (!Count) {
      return std::nullopt;
    }

    Ctr += *Count;
  }

  return Ctr;
}

std::optional<uint64_t>
PAMM::getSumCount(const std::set<std::string> &CounterIds) {
  return getSumCountInternal(*this, CounterIds.begin(), CounterIds.end());
}

std::optional<uint64_t>
PAMM::getSumCount(llvm::ArrayRef<llvm::StringRef> CounterIds) {
  return getSumCountInternal(*this, CounterIds.begin(), CounterIds.end());
}

std::optional<uint64_t>
PAMM::getSumCount(std::initializer_list<llvm::StringRef> CounterIds) {
  return getSumCountInternal(*this, CounterIds.begin(), CounterIds.end());
}

void PAMM::regHistogram(llvm::StringRef HistogramId) {
  auto [It, Inserted] = Histogram.try_emplace(HistogramId);
  assert(Inserted && "failed to register new histogram due to an invalid id");
}

void PAMM::addToHistogram(llvm::StringRef HistogramId,
                          llvm::StringRef DataPointId,
                          uint64_t DataPointValue) {
  auto HistIt = Histogram.find(HistogramId);
  if (HistIt == Histogram.end()) {
    assert(false && "adding data point to histogram failed due to invalid id");
    return;
  }

  auto [DataIt, Inserted] =
      HistIt->second.try_emplace(DataPointId, DataPointValue);
  if (!Inserted) {
    DataIt->second += DataPointValue;
  }
}

void PAMM::stopAllTimers() {
  while (!RunningTimer.empty()) {
    // safe copy
    auto Id = RunningTimer.begin()->first().str();
    stopTimer(Id);
  }
}

void PAMM::printTimers(llvm::raw_ostream &OS) {
  // stop all running timer
  stopAllTimers();

  OS << "Single Timer\n";
  OS << "------------\n";
  for (const auto &Timer : StoppedTimer) {
    uint64_t Time = elapsedTime(Timer.first());
    OS << Timer.first() << " : " << getPrintableDuration(Time) << '\n';
  }
  if (StoppedTimer.empty()) {
    OS << "No single Timer started!\n\n";
  } else {
    OS << "\n";
  }
  OS << "Repeating Timer\n";
  OS << "---------------\n";

  foreachElapsedTimeOfRepeatingTimer(RepeatingTimer,
                                     [&OS](llvm::StringRef Id, auto Handler) {
                                       OS << Id << " Timer:\n";
                                       std::vector<uint64_t> Times;
                                       std::invoke(std::move(Handler), Times);

                                       uint64_t Sum = 0;
                                       for (auto Duration : Times) {
                                         Sum += Duration;
                                         OS << Duration << '\n';
                                       }
                                       OS << "===\n" << Sum << "\n\n";
                                     });

  if (RepeatingTimer.empty()) {
    OS << "No repeating Timer found!\n";
  } else {
    OS << '\n';
  }
}

void PAMM::printCounters(llvm::raw_ostream &OS) {
  OS << "\nCounter\n";
  OS << "-------\n";
  for (const auto &Counter : Counter) {
    OS << Counter.first() << " : " << Counter.second << '\n';
  }
  if (Counter.empty()) {
    OS << "No Counter registered!\n";
  } else {
    OS << "\n";
  }
}

void PAMM::printHistograms(llvm::raw_ostream &OS) {
  OS << "\nHistograms\n";
  OS << "--------------\n";
  for (const auto &H : Histogram) {
    OS << H.first() << " Histogram\n";
    OS << "Value : #Occurrences\n";
    for (const auto &Entry : H.second) {
      OS << Entry.first() << " : " << Entry.second << '\n';
    }
    OS << '\n';
  }
  if (Histogram.empty()) {
    OS << "No histograms tracked!\n";
  }
}

void PAMM::printMeasuredData(llvm::raw_ostream &Os) {
  Os << "\n----- START OF EVALUATION DATA -----\n\n";
  printTimers(Os);
  printCounters(Os);
  printHistograms(Os);
  Os << "\n----- END OF EVALUATION DATA -----\n\n";
}

void PAMM::exportMeasuredData(
    const llvm::Twine &OutputPath, llvm::StringRef ProjectId,
    const std::vector<std::string> *Modules,
    const std::vector<std::string> *DataFlowAnalyses) {
  // json file for holding all data
  json JsonData;

  stopAllTimers();
  {
    // add timer data
    json JTimer;
    for (const auto &Timer : StoppedTimer) {
      uint64_t Time = elapsedTime(Timer.first());
      JTimer[Timer.first().str()] = Time;
    }

    foreachElapsedTimeOfRepeatingTimer(
        RepeatingTimer, [&JTimer](llvm::StringRef Id, auto Handler) {
          std::vector<uint64_t> Times;
          std::invoke(std::move(Handler), Times);
          JTimer[Id.str()] = std::move(Times);
        });

    JsonData["Timer"] = std::move(JTimer);
  }

  {
    // add histogram data if available
    json JHistogram;
    for (const auto &H : Histogram) {
      json JSetH;
      for (const auto &Entry : H.second) {
        JSetH[Entry.first()] = Entry.second;
      }
      JHistogram[H.first()] = std::move(JSetH);
    }
    if (!JHistogram.is_null()) {
      JsonData["Histogram"] = std::move(JHistogram);
    }
  }
  {
    // add counter data
    json JCounter;
    for (const auto &Counter : Counter) {
      JCounter[Counter.first()] = Counter.second;
    }
    JsonData["Counter"] = std::move(JCounter);
  }
  {
    // add analysis/project/source file information if available
    json JInfo;
    JInfo["Project-ID"] = ProjectId;

    if (Modules) {
      JInfo["Module(s)"] = *Modules;
    }
    if (DataFlowAnalyses) {
      JInfo["Data-flow analysis"] = *DataFlowAnalyses;
    }
    if (!JInfo.is_null()) {
      JsonData["Info"] = std::move(JInfo);
    }
  }

  llvm::SmallString<128> Buf;
  OutputPath.toStringRef(Buf);
  if (!Buf.endswith(".json")) {
    Buf.append(".json");
  }

  std::error_code EC;
  llvm::raw_fd_ostream OS(Buf, EC);

  if (EC) {
    throw std::system_error(EC);
  }

  OS << JsonData << '\n';
}

void PAMM::reset() {
  RunningTimer.clear();
  StoppedTimer.clear();
  RepeatingTimer.clear();
  Counter.clear();
  Histogram.clear();
}
} // namespace psr
