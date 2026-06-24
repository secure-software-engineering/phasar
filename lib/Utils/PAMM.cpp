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
#include "phasar/Utils/MapUtils.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <chrono>

using namespace psr;

PAMM &PAMM::getInstance() {
  static PAMM Instance{};
  return Instance;
}

void PAMM::printTimers(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printTimers(OS);
}

static void printCountersImpl(
    llvm::raw_ostream &OS, const pamm::Category &Cat,
    const llvm::DenseMap<llvm::StringRef, pamm::detail::CounterBase *>
        &CatCtrs) {
  OS << Cat.name() << ":\n";
  for (const auto &[Name, C] : CatCtrs) {
    OS << "  " << Name << ": " << C->Ctr << '\n';
  }
  OS << '\n';
}

void pamm::Registry::printCounters(llvm::raw_ostream &OS) const {
  OS << "\nCounters\n";
  OS << "--------\n";

  for (const auto &[Cat, CatCtrs] : Counters) {
    if (Cat->isEnabled()) {
      printCountersImpl(OS, *Cat, CatCtrs);
    }
  }
  if (Counters.empty()) {
    OS << "No Counter registered!\n";
  }
}

void pamm::Registry::printCounters(llvm::raw_ostream &OS,
                                   const Category &Cat) const {
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  const auto *CatCtrs = getOrNull(Counters, &Cat);
  if (!CatCtrs || CatCtrs->empty()) {
    OS << "No Counters for category '" << Cat.name() << "' registered!\n";
    return;
  }

  OS << "\nCounters\n";
  OS << "--------\n";
  printCountersImpl(OS, Cat, *CatCtrs);
}

void PAMM::printCounters(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printCounters(OS);
}

void PAMM::printHistograms(llvm::raw_ostream &OS) {
  pamm::Registry::instance().printHistograms(OS);
}

static void printHistogramsImpl(
    llvm::raw_ostream &OS, const pamm::Category &Cat,
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
  OS << "\nHistograms\n";
  OS << "--------------\n";
  for (const auto &[Cat, Hists] : Histograms) {
    if (Cat->isEnabled()) {
      printHistogramsImpl(OS, *Cat, Hists);
    }
  }
  if (Histograms.empty()) {
    OS << "No histograms tracked!\n";
  }
}

void pamm::Registry::printHistograms(llvm::raw_ostream &OS,
                                     const Category &Cat) const {
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  const auto *Hists = getOrNull(Histograms, &Cat);
  if (!Hists || Hists->empty()) {
    OS << "No Histograms for category '" << Cat.name() << "' registered!\n";
    return;
  }

  OS << "\nHistograms\n";
  OS << "--------------\n";
  printHistogramsImpl(OS, Cat, *Hists);
}

static void printTimersImpl(
    llvm::raw_ostream &OS, const pamm::Category &Cat,
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
  OS << "\nTimers\n";
  OS << "--------------\n";
  for (const auto &[Cat, CatTms] : Timers) {
    if (Cat->isEnabled()) {
      printTimersImpl(OS, *Cat, CatTms);
    }
  }
  if (Histograms.empty()) {
    OS << "No timers tracked!\n";
  }
}

void pamm::Registry::printTimers(llvm::raw_ostream &OS,
                                 const Category &Cat) const {
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  const auto *CatTms = getOrNull(Timers, &Cat);
  if (!CatTms || CatTms->empty()) {
    OS << "No Timers for category '" << Cat.name() << "' registered!\n";
    return;
  }

  OS << "\nTimers\n";
  OS << "--------------\n";
  printTimersImpl(OS, Cat, *CatTms);
}

void pamm::printMeasuredData(llvm::raw_ostream &OS) {
  OS << "\n----- START OF EVALUATION DATA -----\n\n";
  auto &Reg = pamm::Registry::instance();
  Reg.printTimers(OS);
  Reg.printCounters(OS);
  Reg.printHistograms(OS);
  OS << "\n----- END OF EVALUATION DATA -----\n\n";
}

void pamm::printMeasuredData(llvm::raw_ostream &OS, const pamm::Category &Cat) {
  OS << "\n----- START OF EVALUATION DATA -----\n\n";
  scope_exit Pop = [&] { OS << "\n----- END OF EVALUATION DATA -----\n\n"; };
  if (!Cat.isEnabled()) {
    OS << "Category '" << Cat.name() << "' is disabled\n";
    return;
  }
  auto &Reg = pamm::Registry::instance();
  Reg.printTimers(OS, Cat);
  Reg.printCounters(OS, Cat);
  Reg.printHistograms(OS, Cat);
}

auto pamm::Registry::findCategory(llvm::StringRef Name) const
    -> const Category * {
  return RegisteredCategories.lookup(Name);
}
