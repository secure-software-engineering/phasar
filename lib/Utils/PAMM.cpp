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
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/MapUtils.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <chrono>
#include <cstdint>

using namespace psr;

PAMM &PAMM::getInstance() {
  static PAMM Instance{};
  return Instance;
}

template <typename MapTy, typename ImplFn>
static void printAllHelper(llvm::raw_ostream &OS, const MapTy &Map,
                           llvm::StringRef Header, llvm::StringRef Separator,
                           llvm::StringRef EmptyMsg, ImplFn Impl) {
  OS << '\n' << Header << '\n' << Separator << '\n';
  for (const auto &[Cat, Items] : Map) {
    if (Cat->isEnabled()) {
      Impl(OS, *Cat, Items);
    }
  }
  if (Map.empty()) {
    OS << EmptyMsg << '\n';
  }
}

template <typename MapTy, typename ImplFn>
static void printCategoryHelper(llvm::raw_ostream &OS,
                                const pamm::Category<true> &Cat,
                                const MapTy &Map, llvm::StringRef TypeName,
                                llvm::StringRef Separator, ImplFn Impl) {
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  const auto *Items = getOrNull(Map, &Cat);
  if (!Items || Items->empty()) {
    OS << "No " << TypeName << " for category '" << Cat.name()
       << "' registered!\n";
    return;
  }
  OS << '\n' << TypeName << '\n' << Separator << '\n';
  Impl(OS, Cat, *Items);
}

static void printCountersImpl(
    llvm::raw_ostream &OS, const pamm::Category<true> &Cat,
    const llvm::DenseMap<llvm::StringRef, pamm::detail::CounterBase *>
        &CatCtrs) {
  OS << Cat.name() << ":\n";
  for (const auto &[Name, C] : CatCtrs) {
    OS << "  " << Name << ": " << C->Ctr << '\n';
  }
  OS << '\n';
}

void pamm::Registry::printCounters(llvm::raw_ostream &OS) const {
  printAllHelper(OS, Counters, "Counters", "--------", "No Counter registered!",
                 printCountersImpl);
}

void pamm::Registry::printCounters(llvm::raw_ostream &OS,
                                   const Category<true> &Cat) const {
  printCategoryHelper(OS, Cat, Counters, "Counters", "--------",
                      printCountersImpl);
}

static void printMMCountersImpl(
    llvm::raw_ostream &OS, const pamm::Category<true> &Cat,
    const llvm::DenseMap<llvm::StringRef, pamm::detail::MinMaxCounterBase *>
        &CatCtrs) {
  OS << Cat.name() << ":\n";
  for (const auto &[Name, C] : CatCtrs) {
    OS << "  " << Name << ": min(" << C->Min << "), max(" << C->Max
       << "), avg: " << llvm::format("%g", C->Avg.getAverage()) << ", #samples("
       << C->Avg.getNumSamples() << ")\n";
  }
  OS << '\n';
}

void pamm::Registry::printMinMaxCounters(llvm::raw_ostream &OS) const {
  printAllHelper(OS, MMCounters, "Min-Max-Counters", "--------",
                 "No MinMax-Counter registered!", printMMCountersImpl);
}

void pamm::Registry::printMinMaxCounters(llvm::raw_ostream &OS,
                                         const Category<true> &Cat) const {
  printCategoryHelper(OS, Cat, MMCounters, "Min-Max-Counters", "--------",
                      printMMCountersImpl);
}

void PAMM::printCounters(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printCounters(OS);
}

void PAMM::printHistograms(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printHistograms(OS);
}

static void printHistogramsImpl(
    llvm::raw_ostream &OS, const pamm::Category<true> &Cat,
    const llvm::DenseMap<llvm::StringRef, pamm::detail::HistogramBase *>
        &Hists) {
  for (const auto &[Name, H] : Hists) {
    OS << Cat.name() << "::" << Name << " Histogram:\n";
    OS << "  Value\t| #Occurrences\n";
    OS << "  -----\t| ------------\n";
    for (const auto &[Dat, Val] : H->HistData) {
      OS << "  " << Dat << "\t| " << Val << '\n';
    }
    OS << '\n';
  }
}

void pamm::Registry::printHistograms(llvm::raw_ostream &OS) const {
  printAllHelper(OS, Histograms, "Histograms", "--------------",
                 "No histograms tracked!", fn<printHistogramsImpl>);
}

void pamm::Registry::printHistograms(llvm::raw_ostream &OS,
                                     const Category<true> &Cat) const {
  printCategoryHelper(OS, Cat, Histograms, "Histograms", "--------------",
                      fn<printHistogramsImpl>);
}

static void printTimersImpl(
    llvm::raw_ostream &OS, const pamm::Category<true> &Cat,
    const llvm::DenseMap<llvm::StringRef, pamm::detail::TimerBase *> &CatTms) {
  OS << Cat.name() << ":\n";
  for (const auto &[Name, Tm] : CatTms) {
    auto Time = Tm->Acc;
    bool StillRunning = Tm->Tm.has_value();
    OS << "  " << Name << ":\t";
    if (StillRunning) {
      Time += Tm->Tm->elapsedNanos();
      OS << hms{Time} << " (still running)\n";
    } else {
      OS << hms{Time} << '\n';
    }
  }
  OS << '\n';
}

void pamm::Registry::printTimers(llvm::raw_ostream &OS) const {
  printAllHelper(OS, Timers, "Timers", "--------------", "No timers tracked!",
                 fn<printTimersImpl>);
}

void PAMM::printTimers(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printTimers(OS);
}

void pamm::Registry::printTimers(llvm::raw_ostream &OS,
                                 const Category<true> &Cat) const {
  printCategoryHelper(OS, Cat, Timers, "Timers", "--------------",
                      fn<printTimersImpl>);
}

void pamm::printMeasuredData(llvm::raw_ostream &OS) {
  OS << "\n----- START OF EVALUATION DATA -----\n\n";
  auto &Reg = pamm::Registry::instance();
  Reg.printTimers(OS);
  Reg.printCounters(OS);
  Reg.printMinMaxCounters(OS);
  Reg.printHistograms(OS);
  OS << "\n----- END OF EVALUATION DATA -----\n\n";
}

void pamm::printMeasuredData(llvm::raw_ostream &OS,
                             const pamm::Category<true> &Cat) {
  OS << "\n----- START OF EVALUATION DATA -----\n\n";
  scope_exit Pop = [&] { OS << "\n----- END OF EVALUATION DATA -----\n\n"; };
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  auto &Reg = pamm::Registry::instance();
  Reg.printTimers(OS, Cat);
  Reg.printCounters(OS, Cat);
  Reg.printMinMaxCounters(OS, Cat);
  Reg.printHistograms(OS, Cat);
}

void pamm::Registry::reset() noexcept {
  for (const auto &[Cat, Ctrs] : Counters) {
    for (const auto &[_, Ctr] : Ctrs) {
      Ctr->Ctr = 0;
    }
  }

  for (const auto &[Cat, Ctrs] : MMCounters) {
    for (const auto &[_, Ctr] : Ctrs) {
      Ctr->Min = SIZE_MAX;
      Ctr->Max = 0;
      Ctr->Avg = {};
    }
  }

  for (const auto &[Cat, Hists] : Histograms) {
    for (const auto &[_, Hist] : Hists) {
      Hist->HistData.clear();
    }
  }

  for (const auto &[Cat, Tms] : Timers) {
    for (const auto &[_, Tm] : Tms) {
      Tm->reset();
    }
  }
}

void pamm::Registry::clear() noexcept {
  Counters.clear();
  MMCounters.clear();
  Histograms.clear();
  Timers.clear();
  RegisteredCategories.clear();
}

auto pamm::Registry::findCategory(llvm::StringRef Name) const
    -> const Category<true> * {
  return RegisteredCategories.lookup(Name);
}
