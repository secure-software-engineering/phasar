/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPCIPHERCTXDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPCIPHERCTXDESCRIPTION_H_

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/VarStaticRenaming.h"
#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/StringRef.h"

#include <array>

namespace psr {
enum class OpenSSLEVPCIPHERCTXState {
  TOP = 42,
  BOT = 0,
  ALLOCATED,
  INITIALIZED_CIPHER,
  INITIALIZED_ENCRYPT,
  INITIALIZED_DECRYPT,
  FINALIZED,
  FREED,
  ERROR,
  UNINIT
};

class OpenSSLEVPCIPHERCTXDescription
    : public TypeStateDescription<OpenSSLEVPCIPHERCTXState> {

  enum class OpenSSLEVPCIPHERCTXToken {
    EVP_CIPHER_CTX_NEW = 0,
    EVP_CIPHER_INIT,
    EVP_CIPHER_UPDATE,
    EVP_CIPHER_FINAL,
    EVP_ENCRYPT_INIT,
    EVP_ENCRYPT_UPDATE,
    EVP_ENCRYPT_FINAL,
    EVP_DECRYPT_INIT,
    EVP_DECRYPT_UPDATE,
    EVP_DECRYPT_FINAL,
    EVP_CIPHER_CTX_FREE,
    STAR
  };

  // in this API, we don't have situations, where there is more than one
  // interesting argument index
  static const std::array<int, enum2int(OpenSSLEVPCIPHERCTXToken::STAR)>
      OpenSSLEVPCIPHERCTXFuncs;
  static const OpenSSLEVPCIPHERCTXState
      Delta[enum2int(OpenSSLEVPCIPHERCTXToken::STAR) + 1]
           [enum2int(OpenSSLEVPCIPHERCTXState::UNINIT) + 1];

  [[nodiscard]] OpenSSLEVPCIPHERCTXToken
  funcNameToToken(llvm::StringRef F) const;

  const stringstringmap_t *staticRenaming = nullptr;

  llvm::StringMap<OpenSSLEVPCIPHERCTXToken> name2tok;
  const std::string typeNameOfInterest;

public:
  OpenSSLEVPCIPHERCTXDescription(
      const stringstringmap_t *staticRenaming = nullptr,
      llvm::StringRef typenameOfInterest = "evp_cipher_ctx_st");
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

  [[nodiscard]] std::string stateToString(TypeStateDescription::State S) const;

  [[nodiscard]] llvm::StringRef tokenToString(int tok) const;

  [[nodiscard]] llvm::StringRef demangleToken(llvm::StringRef Tok) const;

  [[nodiscard]] TypeStateDescription::State bottom() const override;

  [[nodiscard]] TypeStateDescription::State top() const override;

  [[nodiscard]] TypeStateDescription::State uninit() const override;

  [[nodiscard]] TypeStateDescription::State start() const override;

  [[nodiscard]] TypeStateDescription::State error() const override;

  [[nodiscard]] DataFlowAnalysisType analysisType() const override {
    return DataFlowAnalysisType::IDEOpenSSLCipherTypeStateAnalysis;
  }
};
} // namespace psr
#endif