/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include <map>
#include <set>
#include <string>

namespace psr {

enum class OpenSSLSecureMemoryState;
llvm::StringRef to_string(OpenSSLSecureMemoryState State) noexcept;

class OpenSSLSecureMemoryDescription
    : public TypeStateDescription<OpenSSLSecureMemoryState> {
public:
  using TypeStateDescription::getNextState;
  [[nodiscard]] bool isFactoryFunction(llvm::StringRef F) const override;
  [[nodiscard]] bool isConsumingFunction(llvm::StringRef F) const override;
  [[nodiscard]] bool isAPIFunction(llvm::StringRef F) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(llvm::StringRef Tok,
               TypeStateDescription::State S) const override;
  [[nodiscard]] std::string getTypeNameOfInterest() const override;
  [[nodiscard]] std::set<int>
  getConsumerParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] std::set<int>
  getFactoryParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] TypeStateDescription::State bottom() const override;
  [[nodiscard]] TypeStateDescription::State top() const override;
  [[nodiscard]] TypeStateDescription::State uninit() const override;
  [[nodiscard]] TypeStateDescription::State start() const override;
  [[nodiscard]] TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
