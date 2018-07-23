/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

/*
 * LLVMIFDSSolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#include <algorithm>
#include <map>
#include <string>
#include <iostream> // std::cout

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/MWAIFDSSolver.h>
#include <phasar/PhasarLLVM/Utils/SummaryStrategy.h>
#include <phasar/Utils/Table.h>


namespace psr {

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
        DUMP_RESULTS(dumpResults), Problem(problem) {}

  virtual void solve() override {
    // Solve the analaysis problem
    MWAIFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
                  I>::solve();
    bl::core::get()->flush();
    if (DUMP_RESULTS)
      dumpResults();
  }

  virtual void combine() override {
    std::cout << "LLVMMWAIFDSSolver::combine()\n";
    MWAIFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
                  I>::combine();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    std::cout << "### DUMP LLVMIFDSSolver results\n";
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      std::cout << "EMPTY\n";
    } else {
      std::vector<typename Table<const llvm::Instruction *, const llvm::Value *,
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
          std::cout << "--- IFDS START RESULT RECORD ---\n";
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
               << "\tV:  " << cells[i].v << "\n";
      }
    }
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
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
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str() << "\n";
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
