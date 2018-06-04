/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef LLVMMWAIDESOLVER_H_
#define LLVMMWAIDESOLVER_H_

#include "../../control_flow/ICFG.h"
#include "../../misc/SummaryStrategy.h"
#include "../IDETabulationProblem.h"
#include "MWAIDESolver.h"

namespace psr {

template <typename D, typename V, typename I>
class LLVMMWAIDESolver : public MWAIDESolver<const llvm::Instruction *, D,
                                             const llvm::Function *, V, I> {
private:
  const bool DUMP_RESULTS;
  IDETabulationProblem<const llvm::Instruction *, D, const llvm::Function *, V,
                       I> &Problem;

public:
  LLVMMWAIDESolver(IDETabulationProblem<const llvm::Instruction *, D,
                                        const llvm::Function *, V, I> &problem,
                   enum SummaryGenerationStrategy S, bool dumpResults = false)
      : IDESolver<const llvm::Instruction *, D, const llvm::Function *, V, I>(
            problem, S),
        DUMP_RESULTS(dumpResults), Problem(problem) {}

  virtual ~LLVMMWAIDESolver() = default;

  void solve() override {
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void combine() override {
    cout << "LLVMMWAIDESolver::combine()\n";
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::combine();
  }

  void dumpResults() {
    cout << "### DUMP LLVMIDESolver results\n";
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      cout << "EMPTY" << endl;
    } else {
      vector<typename Table<const llvm::Instruction *, const llvm::Value *,
                            const llvm::Value *>::Cell>
          cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Value *>::Cell a,
              typename Table<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Value *>::Cell b) {
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
        cout << "D:\t";
        if (cells[i].c == nullptr)
          cout << "  nullptr " << endl;
        else
          cout << Problem.DtoString(cells[i].c) << " "
               << "\tV:  " << Problem.VtoString(cells[i].v) << "\n";
      }
    }
    //		cout << "### IDE RESULTS AT LAST STATEMENT OF MAIN" << endl;
    //		auto resultAtEnd =
    // this->resultsAt(this->icfg.getLastInstructionOf("main"));
    //		for (auto entry : resultAtEnd) {
    //			cout << "\t--- begin entry ---" << endl;
    //			entry.first->dump();
    //			cout << entry.second << endl;
    //			cout << "\t--- end entry ---" << endl;
    //		}
    //		cout << "### IDE END RESULTS AT LAST STATEMENT OF MAIN" << endl;
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
};

} // namespace psr

#endif
