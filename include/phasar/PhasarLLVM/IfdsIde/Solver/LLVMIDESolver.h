/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIDESolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_H_
#define ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_H_

#include "IDESolver.h"
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>

template <typename D, typename V, typename I>
class LLVMIDESolver : public IDESolver<const llvm::Instruction *, D,
                                       const llvm::Function *, V, I> {
private:
  const bool DUMP_RESULTS;
  IDETabulationProblem<const llvm::Instruction *, D, const llvm::Function *, V,
                       I> &Problem;

public:
  LLVMIDESolver(IDETabulationProblem<const llvm::Instruction *, D,
                                     const llvm::Function *, V, I> &problem,
                bool dumpResults = false)
      : IDESolver<const llvm::Instruction *, D, const llvm::Function *, V, I>(
            problem),
        DUMP_RESULTS(dumpResults), Problem(problem) {}

  virtual ~LLVMIDESolver() = default;

  void solve() override {
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    cout << "### DUMP LLVMIDESolver results\n";
    // for the following line have a look at:
    // http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
    // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      cout << "EMPTY" << endl;
    } else {
      vector<typename Table<const llvm::Instruction *, D, V>::Cell> cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<const llvm::Instruction *, D, V>::Cell a,
              typename Table<const llvm::Instruction *, D, V>::Cell b) {
             return a.r < b.r;
           });
      const llvm::Instruction *prev = nullptr;
      const llvm::Instruction *curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        if (prev != curr) {
          prev = curr;
          cout << "--- IDE START RESULT RECORD ---\n";
          cout << "N: " << Problem.NtoString(cells[i].r) << " in function: ";
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
            cout << inst->getFunction()->getName().str() << "\n";
          }
        }
        cout << "D:\t" << Problem.DtoString(cells[i].c) << " "
             << "\tV:  " << Problem.VtoString(cells[i].v) << "\n";
      }
    }
  }

  void dumpAllInterPathEdges() {
    cout << "COMPUTED INTER PATH EDGES" << endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      cout << "FROM" << endl;
      cell.r->dump();
      cout << "TO" << endl;
      cell.c->dump();
      cout << "FACTS" << endl;
      for (auto &fact : cell.v) {
        cout << "fact" << endl;
        fact.first->dump();
        cout << "produces" << endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    cout << "COMPUTED INTRA PATH EDGES" << endl;
    auto intrape = this->computedIntraPathEdges.cellSet();
    for (auto &cell : intrape) {
      cout << "FROM" << endl;
      cell.r->dump();
      cout << "TO" << endl;
      cell.c->dump();
      cout << "FACTS" << endl;
      for (auto &fact : cell.v) {
        cout << "fact" << endl;
        fact.first->dump();
        cout << "produces" << endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void exportPATBCJSON() { cout << "LLVMIDESolver::exportPATBCJSON()\n"; }
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_LLVMIDESOLVER_HH_ */
