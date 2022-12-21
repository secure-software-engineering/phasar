/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DOMAIN_LLVMANALYSISDOMAIN_H
#define PHASAR_PHASARLLVM_DOMAIN_LLVMANALYSISDOMAIN_H

#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

namespace llvm {
class Value;
class Instruction;
class StructType;
class Function;
} // namespace llvm

namespace psr {
enum class BinaryDomain;

struct LLVMAnalysisDomainDefault : public AnalysisDomain {
  using d_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using c_t = LLVMBasedCFG;
  using i_t = LLVMBasedICFG;
  using db_t = LLVMProjectIRDB;
};

struct LLVMIFDSAnalysisDomainDefault : LLVMAnalysisDomainDefault {
  using l_t = BinaryDomain;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DOMAIN_LLVMANALYSISDOMAIN_H
