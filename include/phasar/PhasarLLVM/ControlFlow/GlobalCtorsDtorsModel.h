/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_GLOBALCTORSDTORSMODEL_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_GLOBALCTORSDTORSMODEL_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"

namespace psr {
class LLVMProjectIRDB;

class GlobalCtorsDtorsModel {
public:
  static constexpr llvm::StringLiteral ModelName =
      "__psrCRuntimeGlobalCtorsModel";

  static constexpr llvm::StringLiteral DtorModelName =
      "__psrCRuntimeGlobalDtorsModel";

  static constexpr llvm::StringLiteral DtorsCallerName =
      "__psrGlobalDtorsCaller";

  static constexpr llvm::StringLiteral UserEntrySelectorName =
      "__psrCRuntimeUserEntrySelector";

  static llvm::Function *
  buildModel(LLVMProjectIRDB &IRDB,
             llvm::ArrayRef<llvm::Function *> UserEntryPoints);
  static llvm::Function *
  buildModel(LLVMProjectIRDB &IRDB,
             llvm::ArrayRef<std::string> UserEntryPoints);

  /// Returns true, if a function was generated by phasar.
  [[nodiscard]] static bool isPhasarGenerated(const llvm::Function &F) noexcept;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_GLOBALCTORSDTORSMODEL_H
