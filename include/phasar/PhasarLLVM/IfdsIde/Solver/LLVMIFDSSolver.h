/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIFDSSolver.h
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIFDSSOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_LLVMIFDSSOLVER_H_

#include <algorithm>
#include <map>
#include <string>

#include <json.hpp>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/IFDSSolver.h>
#include <phasar/Utils/PAMMMacros.h>
#include <phasar/Utils/Table.h>

using json = nlohmann::json;

namespace psr {

template <typename D, typename I>
class LLVMIFDSSolver : public IFDSSolver<const llvm::Instruction *, D,
                                         const llvm::Function *, I> {
private:
  IFDSTabulationProblem<const llvm::Instruction *, D, const llvm::Function *, I>
      &Problem;
  const bool PRINT_REPORT;

public:
  ~LLVMIFDSSolver() override = default;

  LLVMIFDSSolver(IFDSTabulationProblem<const llvm::Instruction *, D,
                                       const llvm::Function *, I> &problem,
                 bool printReport = true)
      : IFDSSolver<const llvm::Instruction *, D, const llvm::Function *, I>(
            problem),
        Problem(problem), PRINT_REPORT(printReport) {}

  void solve() override {
    // Solve the analaysis problem
    IFDSSolver<const llvm::Instruction *, D, const llvm::Function *,
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
        std::cout, IFDSSolver<const llvm::Instruction *, D,
                              const llvm::Function *, I>::getSolverResults());
  }

  void dumpResults() {
    PAMM_GET_INSTANCE;
    START_TIMER("DFA IFDS Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    std::cout
        << "\n**************************************************************\n"
        << "*                 Raw LLVMIFDSSolver results                 *\n"
        << "**************************************************************\n\n"
        << "========== Raw LLVMIFDSSolver results ==========\n\nThe value of "
           "all facts is Bottom - Top values are not shown!\n";
    auto cells = this->valtab.cellVec();
    if (cells.empty()) {
      std::cout << "No results computed!\n";
    } else {
      llvmValueIDLess llvmIDLess;
      sort(cells.begin(), cells.end(),
           [&llvmIDLess](
               typename Table<const llvm::Instruction *, D, BinaryDomain>::Cell
                   a,
               typename Table<const llvm::Instruction *, D, BinaryDomain>::Cell
                   b) {
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
        std::cout << "\tD: " << Problem.DtoString(cells[i].c) << '\n';
      }
    }
    std::cout << '\n';
    STOP_TIMER("DFA IFDS Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      cell.r->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str()
                << "\n";
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str()
                << "\n";
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
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str()
                << "\n";
      std::cout << "TO" << std::endl;
      cell.c->dump();
      std::cout << "IN FUNCTION: " << cell.r->getFunction()->getName().str()
                << "\n";
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

  void exportPATBCJSON() { std::cout << "LLVMIFDSSolver::exportPATBCJSON()\n"; }
};
} // namespace psr

#endif
