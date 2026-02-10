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

#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/LLVMAnalysisPrinter.h"
#include "phasar/Utils/DefaultAnalysisPrinterSelector.h"
#include "phasar/Utils/TypeTraits.h"

namespace llvm {
class Value;
class Instruction;
class StructType;
class Function;
class DIType;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;
class LLVMBasedICFG;
class LLVMBasedCFG;

struct LLVMIRDomain {
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using v_t = const llvm::Value *;
  using t_t = const llvm::DIType *;
  using db_t = LLVMProjectIRDB;
};

/// \brief An AnalysisDomain that specializes sensible defaults for LLVM-based
/// analysis
struct LLVMAnalysisDomainDefault : public AnalysisDomain {
  using ir_t = LLVMIRDomain;
  using n_t = typename ir_t::n_t;
  using f_t = typename ir_t::f_t;
  using t_t = typename ir_t::t_t;
  using v_t = typename ir_t::v_t;
  using db_t = typename ir_t::db_t;

  using d_t = const llvm::Value *;
  using c_t = LLVMBasedCFG;
  using i_t = LLVMBasedICFG;
};

/// \brief An AnalysisDomain that specializes sensible defaults for LLVM-based
/// IFDS analysis
using LLVMIFDSAnalysisDomainDefault =
    WithBinaryValueDomain<LLVMAnalysisDomainDefault>;

extern template class DefaultLLVMAnalysisPrinter<LLVMIFDSAnalysisDomainDefault>;

template <>
struct DefaultAnalysisPrinterSelector<LLVMIFDSAnalysisDomainDefault>
    : type_identity<DefaultLLVMAnalysisPrinter<LLVMIFDSAnalysisDomainDefault>> {
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DOMAIN_LLVMANALYSISDOMAIN_H
