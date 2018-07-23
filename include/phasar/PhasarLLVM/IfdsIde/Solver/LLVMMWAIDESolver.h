/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#pragma message "Will break as definition of llvm::Instruction, llvm::Function, ... are not given"

#include <iostream>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM//Utils/SummaryStrategy.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/MWAIDESolver.h>

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
    std::cout << "LLVMMWAIDESolver::combine()\n";
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::combine();
  }

  void dumpResults() {
    std::cout << "### DUMP LLVMIDESolver results\n";
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      std::cout << "EMPTY" << std::endl;
    } else {
      std::vector<typename Table<const llvm::Instruction *, const llvm::Value *,
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
          std::cout << "--- IDE START RESULT RECORD ---\n";
          std::cout << "N: " << Problem.NtoString(cells[i].r) << " in function: ";
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(cells[i].r)) {
            std::cout << inst->getFunction()->getName().str() << "\n";
          }
        }
        std::cout << "D:\t";
        if (cells[i].c == nullptr)
          std::cout << "  nullptr " << std::endl;
        else
          std::cout << Problem.DtoString(cells[i].c) << " "
               << "\tV:  " << Problem.VtoString(cells[i].v) << "\n";
      }
    }
    //		std::cout << "### IDE RESULTS AT LAST STATEMENT OF MAIN" << std::endl;
    //		auto resultAtEnd =
    // this->resultsAt(this->icfg.getLastInstructionOf("main"));
    //		for (auto entry : resultAtEnd) {
    //			std::cout << "\t--- begin entry ---" << std::endl;
    //			entry.first->dump();
    //			std::cout << entry.second << std::endl;
    //			std::cout << "\t--- end entry ---" << std::endl;
    //		}
    //		std::cout << "### IDE END RESULTS AT LAST STATEMENT OF MAIN" << std::endl;
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        fact.first->dump();
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    std::cout << "COMPUTED INTRA PATH EDGES" << std::endl;
    auto intrape = this->computedIntraPathEdges.cellSet();
    for (auto &cell : intrape) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        fact.first->dump();
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          out->dump();
        }
      }
    }
  }
};

} // namespace psr
