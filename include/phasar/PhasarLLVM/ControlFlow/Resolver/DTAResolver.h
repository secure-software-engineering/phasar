/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DTAResolver.h
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_DTARESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_DTARESOLVER_H_

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h"
// To switch the TypeGraph
//#include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"

#include <string>

namespace llvm {
class Instruction;
class CallBase;
class Function;
class BitCastInst;
} // namespace llvm

namespace psr {

class DTAResolver : public CHAResolver {
public:
  using TypeGraph_t = CachedTypeGraph;

protected:
  TypeGraph_t TypeGraph;

  /**
   * An heuristic that return true if the bitcast instruction is interesting to
   * take into the DTA relational graph
   */
  static bool
  heuristicAntiConstructorThisType(const llvm::BitCastInst *BitCast);

  /**
   * Another heuristic that return true if the bitcast instruction is
   * interesting to take into the DTA relational graph (use the presence or not
   * of vtable)
   */
  bool heuristicAntiConstructorVtablePos(const llvm::BitCastInst *BitCast);

public:
  DTAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  ~DTAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  void otherInst(const llvm::Instruction *Inst) override;

  [[nodiscard]] std::string str() const override;
};
} // namespace psr

#endif
