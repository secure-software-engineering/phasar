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

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/StringRef.h>
#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/Utils/EnumFlags.h"
#include <llvm/ADT/StringMap.h>
namespace psr {

class OpenSSLEVPMDCTXDescription : public TypeStateDescription {
  // TODO: We don't check whether the EVP_MD object is properly instantiated
  // when passed to EVP_Digest[Sign]Init[_ex]

  enum OpenSSLEVPMDCTXState {
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
      llvm::StringRef typenameOfInterest = "__forward_tag_reference_14");
  bool isFactoryFunction(const std::string &F) const override;

  bool isConsumingFunction(const std::string &F) const override;

  bool isAPIFunction(const std::string &F) const override;

  TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;

  std::string getTypeNameOfInterest() const override;

  std::set<int> getConsumerParamIdx(const std::string &F) const override;

  std::set<int> getFactoryParamIdx(const std::string &F) const override;

  std::string stateToString(TypeStateDescription::State S) const override;

  llvm::StringRef
  stateToUnownedString(TypeStateDescription::State S) const override;

  llvm::StringRef tokenToString(int Tok) const override;

  llvm::StringRef demangleToken(llvm::StringRef Tok) const override;

  TypeStateDescription::State bottom() const override;

  TypeStateDescription::State top() const override;

  TypeStateDescription::State uninit() const override;

  TypeStateDescription::State start() const override;

  TypeStateDescription::State error() const override;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPMDCTXDESCRIPTION_H_