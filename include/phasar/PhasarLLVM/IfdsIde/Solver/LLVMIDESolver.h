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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIDESOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIDESOLVER_H_

#include <algorithm>
#include <iostream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/IDESolver.h>

using json = nlohmann::json;

namespace psr {

template <typename D, typename V, typename I>
class LLVMIDESolver : public IDESolver<const llvm::Instruction *, D,
                                       const llvm::Function *, V, I> {
private:
  IDETabulationProblem<const llvm::Instruction *, D, const llvm::Function *, V,
                       I> &Problem;
  const bool PRINT_REPORT;

public:
  LLVMIDESolver(IDETabulationProblem<const llvm::Instruction *, D,
                                     const llvm::Function *, V, I> &problem,
                bool printReport = true)
      : IDESolver<const llvm::Instruction *, D, const llvm::Function *, V, I>(
            problem),
        Problem(problem), PRINT_REPORT(printReport) {}

  ~LLVMIDESolver() override = default;

  void solve() override {
    IDESolver<const llvm::Instruction *, D, const llvm::Function *, V,
              I>::solve();
    boost::log::core::get()->flush();
    if (PhasarConfig::VariablesMap().count("emit-raw-results")) {
      dumpResults();
    }
    if (PRINT_REPORT) {
      printReport();
    }
  }

  void printReport() {
    Problem.emitTextReport(
        std::cout, IDESolver<const llvm::Instruction *, D,
                             const llvm::Function *, V, I>::getSolverResults());
  }

  void dumpResults() {
    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    std::cout
        << "\n***************************************************************\n"
        << "*                  Raw LLVMIDESolver results                  *\n"
        << "***************************************************************\n";
    auto cells = this->valtab.cellVec();
    if (cells.empty()) {
      std::cout << "No results computed!" << std::endl;
    } else {
      llvmValueIDLess llvmIDLess;
      std::sort(cells.begin(), cells.end(),
                [&llvmIDLess](
                    typename Table<const llvm::Instruction *, D, V>::Cell a,
                    typename Table<const llvm::Instruction *, D, V>::Cell b) {
                  if (!llvmIDLess(a.r, b.r) && !llvmIDLess(b.r, a.r)) {
                    if constexpr (std::is_same<D, const llvm::Value *>::value) {
                      return llvmIDLess(a.c, b.c);
                    } else {
                      // If D is user defined we should use the user defined
                      // less-than comparison
                      return a.c < b.c;
                    }
                  }
                  return llvmIDLess(a.r, b.r);
                });
      const llvm::Instruction *prev = nullptr;
      const llvm::Instruction *curr;
      const llvm::Function *prevFn = nullptr;
      const llvm::Function *currFn;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        currFn = curr->getFunction();
        if (prevFn != currFn) {
          prevFn = currFn;
          std::cout << "\n\n============ Results for function '" +
                           currFn->getName().str() + "' ============\n";
        }
        if (prev != curr) {
          prev = curr;
          std::string NString = Problem.NtoString(curr);
          std::string line(NString.size(), '-');
          std::cout << "\n\nN: " << NString << "\n---" << line << '\n';
        }
        std::cout << "\tD: " << Problem.DtoString(cells[i].c)
                  << " | V: " << Problem.VtoString(cells[i].v) << '\n';
      }
    }
    std::cout << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
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

  void exportPATBCJSON() { std::cout << "LLVMIDESolver::exportPATBCJSON()\n"; }
};
} // namespace psr

#endif
