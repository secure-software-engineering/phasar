/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_ANALYSISDOMAIN_H_
#define PHASAR_PHASARLLVM_IFDSIDE_ANALYSISDOMAIN_H_

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {
class LLVMBasedICFG;

struct AnalysisDomain {
  using d_t = void;
  using n_t = void;
  using f_t = void;
  using t_t = void;
  using v_t = void;

  // type of the element contained in the sets of edge functions
  using e_t = void;
  using l_t = void;
  using i_t = void;
};

struct LLVMAnalysisDomainDefault : public AnalysisDomain {
  using d_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;

  using i_t = LLVMBasedICFG;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_ANALYSISDOMAIN_H_
