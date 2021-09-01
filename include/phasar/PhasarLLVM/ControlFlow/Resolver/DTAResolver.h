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

#include <set>
#include <string>

#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h"
// To switch the TypeGraph
//#include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"

namespace llvm {
class Instruction;
class CallBase;
class Function;
} // namespace llvm

namespace psr {

class DTAResolver : public CHAResolver {
public:
  using TypeGraph_t = CachedTypeGraph;

protected:
  TypeGraph_t typegraph;

  /**
   * An heuristic that return true if the bitcast instruction is interesting to
   * take into the DTA relational graph
   */
  static bool
  heuristicAntiConstructorThisType(const llvm::BitCastInst *bitcast);

  /**
   * Another heuristic that return true if the bitcast instruction is
   * interesting to take into the DTA relational graph (use the presence or not
   * of vtable)
   */
  bool heuristicAntiConstructorVtablePos(const llvm::BitCastInst *bitcast);

public:
  DTAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  ~DTAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  void otherInst(const llvm::Instruction *Inst) override;
};
} // namespace psr

#endif
