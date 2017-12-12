/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef LLVMMWAIFDSSOLVER_H_
#define LLVMMWAIFDSSOLVER_H_

/*
 * LLVMIFDSSolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#include <algorithm>
#include <map>
#include <string>
#include "../../../utils/Table.h"
#include "../../control_flow/ICFG.h"
#include "../../misc/SummaryStrategy.h"
#include "../IFDSTabulationProblem.h"
#include "MWAIFDSSolver.h"

using namespace std;

template <typename D, typename I>
class LLVMMWAIFDSSolver : public MWAIFDSSolver<const llvm::Instruction *, D,
                                               const llvm::Function *, I> {
 private:
  const bool DUMP_RESULTS;
  IFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I>
      &Problem;

 public:
  virtual ~LLVMMWAIFDSSolver() = default;

  LLVMMWAIFDSSolver(IFDSTabulationProblem<const llvm::Instruction *, D,
                                          const llvm::Function *, I> &problem,
                    enum SummaryGenerationStrategy S, bool dumpResults = false)
      : MWAIFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>(
            problem, S),
        DUMP_RESULTS(dumpResults),
        Problem(problem) {}

  virtual void solve() override {
    // Solve the analaysis problem
    MWAIFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
                  I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS) dumpResults();
  }

  virtual void combine() override {
    cout << "LLVMMWAIFDSSolver::combine()\n";
    MWAIFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
                  I>::combine();
    if (DUMP_RESULTS) dumpResults();
  }

  void dumpResults() {
    cout << "### DUMP LLVMIFDSSolver results\n";
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      cout << "EMPTY\n";
    } else {
      vector<typename Table<const llvm::Instruction *, const llvm::Value *,
                            BinaryDomain>::Cell>
          cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<const llvm::Instruction *, const llvm::Value *,
                             BinaryDomain>::Cell a,
              typename Table<const llvm::Instruction *, const llvm::Value *,
                             BinaryDomain>::Cell b) { return a.r < b.r; });
      const llvm::Instruction *prev = nullptr;
      const llvm::Instruction *curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        if (prev != curr) {
          prev = curr;
          cout << "--- IFDS START RESULT RECORD ---\n";
          cout << "N: " << Problem.NtoString(cells[i].r) << " in function: ";
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
            cout << inst->getFunction()->getName().str() << "\n";
          }
        }
        cout << "D:\t";
        if (cells[i].c == nullptr)
          cout << "  nullptr " << endl;
        else
          cout << Problem.DtoString(cells[i].c) << " "
               << "\tV:  " << cells[i].v << "\n";
      }
    }
  }

  void dumpAllInterPathEdges() {
    cout << "COMPUTED INTER PATH EDGES" << endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      cout << "FROM" << endl;
      cell.r->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "TO" << endl;
      cell.c->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
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
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      cout << "TO" << endl;
      cell.c->dump();
      cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
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
};

#endif
