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

#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>
#include <phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h>
// To switch the TypeGraph
//#include <phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h>

namespace llvm {
class Instruction;
class ImmutableCallSite;
} // namespace llvm

namespace psr {
class ProjectIRDB;

struct DTAResolver : public CHAResolver {
public:
  using TypeGraph_t = CachedTypeGraph;

protected:
  TypeGraph_t typegraph;
  std::set<const llvm::StructType *> unsound_types;

  /**
   * An heuristic that return true if the bitcast instruction is interesting to
   * take into the DTA relational graph
   */
  bool heuristic_anti_contructor_this_type(const llvm::BitCastInst *bitcast);

  /**
   * Another heuristic that return true if the bitcast instruction is
   * interesting to take into the DTA relational graph (use the presence or not
   * of vtable)
   */
  bool heuristic_anti_contructor_vtable_pos(const llvm::BitCastInst *bitcast);

public:
  DTAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch);
  virtual ~DTAResolver() = default;

  virtual void firstFunction(const llvm::Function *F) override;
  virtual void OtherInst(const llvm::Instruction *Inst) override;
  virtual std::set<std::string>
  resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
};
} // namespace psr

#endif
