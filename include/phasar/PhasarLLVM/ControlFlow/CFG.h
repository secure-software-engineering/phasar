/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CFG.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_CFG_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace llvm {
class raw_ostream;
}

namespace psr {

enum class SpecialMemberFunctionType {
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE) TYPE,
#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.def"
};

std::string toString(const SpecialMemberFunctionType &SMFT);

SpecialMemberFunctionType toSpecialMemberFunctionType(const std::string &SMFT);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const SpecialMemberFunctionType &SMFT);

template <typename N, typename F> class CFG {
public:
  virtual ~CFG() = default;

  virtual F getFunctionOf(N Inst) const = 0;

  virtual std::vector<N> getPredsOf(N Inst) const = 0;

  virtual std::vector<N> getSuccsOf(N Inst) const = 0;

  virtual std::vector<std::pair<N, N>> getAllControlFlowEdges(F Fun) const = 0;

  virtual std::vector<N> getAllInstructionsOf(F Fun) const = 0;

  virtual std::set<N> getStartPointsOf(F Fun) const = 0;

  virtual std::set<N> getExitPointsOf(F Fun) const = 0;

  virtual bool isCallSite(N Inst) const = 0;

  virtual bool isExitInst(N Inst) const = 0;

  virtual bool isStartPoint(N Inst) const = 0;

  virtual bool isFieldLoad(N Inst) const = 0;

  virtual bool isFieldStore(N Inst) const = 0;

  virtual bool isFallThroughSuccessor(N Inst, N Succ) const = 0;

  virtual bool isBranchTarget(N Inst, N Succ) const = 0;

  virtual bool isHeapAllocatingFunction(F Fun) const = 0;

  virtual bool isSpecialMemberFunction(F Fun) const = 0;

  virtual SpecialMemberFunctionType
  getSpecialMemberFunctionType(F Fun) const = 0;

  virtual std::string getStatementId(N Inst) const = 0;

  virtual std::string getFunctionName(F Fun) const = 0;

  virtual std::string getDemangledFunctionName(F Fun) const = 0;

  virtual void print(F Fun, llvm::raw_ostream &OS) const = 0;

  virtual nlohmann::json getAsJson(F Fun) const = 0;
};

} // namespace psr

#endif
