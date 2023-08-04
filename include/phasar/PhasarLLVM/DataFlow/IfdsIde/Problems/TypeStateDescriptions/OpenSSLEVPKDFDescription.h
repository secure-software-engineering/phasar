/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All righ reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include <map>
#include <set>
#include <string>

namespace psr {

/**
 * We use the following lattice
 *                BOT = all information
 *
 *         UNINIT     KDF_FETCHED     ERROR
 *
 *                TOP = no information
 */
enum class OpenSSLEVPKDFState {
  TOP = 42,
  UNINIT = 0,
  KDF_FETCHED = 1,
  ERROR = 2,
  BOT = 3
};

llvm::StringRef to_string(OpenSSLEVPKDFState State) noexcept;

class OpenSSLEVPKDFDescription
    : public TypeStateDescription<OpenSSLEVPKDFState> {
public:
  /**
   * The STAR token represents all functions besides EVP_KDF_fetch(),
   * EVP_KDF_fetch()  and EVP_KDF_CTX_free().
   */
  enum class OpenSSLEVTKDFToken {
    EVP_KDF_FETCH = 0,
    EVP_KDF_FREE = 1,
    STAR = 2
  };

  using State = OpenSSLEVPKDFState;

private:
  // delta matrix to implement the state machine's delta function
  static const OpenSSLEVPKDFState Delta[3][4];
  static OpenSSLEVTKDFToken funcNameToToken(llvm::StringRef F);

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
