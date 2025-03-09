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
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
// To switch the TypeGraph
// #include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"

#include <string>

namespace llvm {
class Instruction;
class CallBase;
class Function;
class BitCastInst;
} // namespace llvm

namespace psr {

class [[deprecated("Does not work with opaque pointers anymore")]] DTAResolver
    : public CHAResolver {
public:
  using TypeGraph_t = CachedTypeGraph;

  DTAResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
              const DIBasedTypeHierarchy *TH);

  ~DTAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  void otherInst(const llvm::Instruction *Inst) override;

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool mutatesHelperAnalysisInformation()
      const noexcept override {
    return false;
  }

protected:
  TypeGraph_t TypeGraph;

  /**
   * An heuristic that return true if the bitcast instruction is interesting to
   * take into the DTA relational graph
   */
  static bool heuristicAntiConstructorThisType(
      const llvm::BitCastInst *BitCast);

  /**
   * Another heuristic that return true if the bitcast instruction is
   * interesting to take into the DTA relational graph (use the presence or not
   * of vtable)

   */
  bool heuristicAntiConstructorVtablePos(const llvm::BitCastInst *BitCast);
};
} // namespace psr

#endif
