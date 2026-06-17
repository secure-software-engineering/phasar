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

  PAMM::TimePoint_t Start = std::chrono::steady_clock::now();
  It->second = Start;
}

void PAMM::resetTimer(llvm::StringRef TimerId) {
  [[maybe_unused]] bool InRunningTimers = RunningTimer.erase(TimerId);
  [[maybe_unused]] bool InStoppedTimers = StoppedTimer.erase(TimerId);

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
    PAMM::TimePoint_t End = std::chrono::steady_clock::now();
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
    PAMM::TimePoint_t End = std::chrono::steady_clock::now();
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
  return hms(Duration_t{Duration}).str();
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

void pamm::Registry::printCounters(llvm::raw_ostream &OS) const {
  OS << "\nCounters\n";
  OS << "--------\n";

  for (const auto &[Cat, CatCtrs] : Counters) {
    if (!Cat->isEnabled()) {
      continue;
    }
    OS << *Cat << ":\n";
    for (const auto &[Name, C] : CatCtrs) {
      OS << "  " << Name << ": " << C->Ctr << '\n';
    }
    OS << '\n';
  }
  if (Counters.empty()) {
    OS << "No Counter registered!\n";
  }
}

void PAMM::printCounters(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printCounters(OS);
}

void PAMM::printHistograms(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printHistograms(OS);
}

void pamm::Registry::printHistograms(llvm::raw_ostream &OS) const {
  OS << "\nHistograms\n";
  OS << "--------------\n";
  for (const auto &[Cat, Hists] : Histograms) {
    if (!Cat->isEnabled()) {
      continue;
    }
    for (const auto &[Name, H] : Hists) {
      OS << Cat->name() << "::" << Name << " Histogram:\n";
      OS << "  Value\t| #Occurrences\n";
      OS << "  -----\t| ------------\n";
      for (const auto &[Dat, Val] : H->HistData) {
        OS << "  " << Dat << "\t| " << Val << '\n';
      }
      OS << '\n';
    }
  }
  if (Histograms.empty()) {
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
  if (!llvm::StringRef(Buf).ends_with(".json")) {
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
