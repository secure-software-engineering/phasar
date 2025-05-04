/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPMDCTXDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPMDCTXDESCRIPTION_H_

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/VarStaticRenaming.h"
#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/StringMap.h"

#include <map>
#include <set>
#include <string>

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/StringRef.h>
namespace psr {

enum class OpenSSLEVPMDCTXState {
  TOP = 42,
  BOT = 0,
  ALLOCATED,
  INITIALIZED,
  SIGN_INITIALIZED,
  FINALIZED,
  FREED,
  ERROR,
  UNINIT,
};

class OpenSSLEVPMDCTXDescription
    : public TypeStateDescription<OpenSSLEVPMDCTXState> {
  // TODO: We don't check whether the EVP_MD object is properly instantiated
  // when passed to EVP_Digest[Sign]Init[_ex]

  // typename: evp_md_ctx_st, evp_md_st
  enum class OpenSSLEVPMDCTXToken {
    EVP_MD_CTX_NEW = 0,
    EVP_DIGEST_INIT,
    EVP_DIGEST_UPDATE,
    EVP_DIGEST_FINAL,
    EVP_DIGEST_SIGN_INIT,
    EVP_DIGEST_SIGN_UPDATE,
    EVP_DIGEST_SIGN_FINAL,
    EVP_MD_CTX_FREE,
    STAR,
  };

  // in this API, we don't have situations, where there is more than one
  // interesting argument index
  static const std::array<int, enum2int(OpenSSLEVPMDCTXToken::STAR)>
      OpenSSLEVPMDCTXFuncs;
  static const OpenSSLEVPMDCTXState
      Delta[enum2int(OpenSSLEVPMDCTXToken::STAR) + 1]
           [enum2int(OpenSSLEVPMDCTXState::UNINIT) + 1];

  OpenSSLEVPMDCTXToken funcNameToToken(llvm::StringRef F) const;
  const stringstringmap_t *staticRenaming = nullptr;

  llvm::StringMap<OpenSSLEVPMDCTXToken> name2tok;
  const std::string typeNameOfInterest;

public:
  OpenSSLEVPMDCTXDescription(
      const stringstringmap_t *staticRenaming = nullptr,
      llvm::StringRef typenameOfInterest = "evp_md_ctx_st");
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

  [[nodiscard]] llvm::StringRef tokenToString(int Tok) const;

  [[nodiscard]] llvm::StringRef demangleToken(llvm::StringRef Tok) const;

  [[nodiscard]] TypeStateDescription::State bottom() const override;

  [[nodiscard]] TypeStateDescription::State top() const override;

  [[nodiscard]] TypeStateDescription::State uninit() const override;

  [[nodiscard]] TypeStateDescription::State start() const override;

  [[nodiscard]] TypeStateDescription::State error() const override;

  [[nodiscard]] DataFlowAnalysisType analysisType() const override {
    return DataFlowAnalysisType::IDEOpenSSLMDTypeStateAnalysis;
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPMDCTXDESCRIPTION_H_