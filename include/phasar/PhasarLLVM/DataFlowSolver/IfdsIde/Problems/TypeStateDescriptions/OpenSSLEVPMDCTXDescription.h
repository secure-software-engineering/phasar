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
#include "phasar/Utils/EnumFlags.h"
namespace psr {

class OpenSSLEVPMDCTXDescription : public TypeStateDescription {
  enum OpenSSLEVPMDCTXState {
    TOP = 42,
    BOT = 0,
    ALLOCATED = 1,
    INITIALIZED = 2,
    FINALIZED = 3,
    FREED = 4,
    ERROR = 5,
    UNINIT = 6,
  };
  // typename: evp_md_ctx_st, evp_md_st
  enum class OpenSSLEVPMDCTXToken {
    EVP_MD_CTX_NEW = 0,
    EVP_DIGEST_INIT,
    EVP_DIGEST_UPDATE,
    EVP_DIGEST_FINAL,
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

  static OpenSSLEVPMDCTXToken funcNameToToken(llvm::StringRef F);

public:
  bool isFactoryFunction(const std::string &F) const override;

  bool isConsumingFunction(const std::string &F) const override;

  bool isAPIFunction(const std::string &F) const override;

  TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;

  std::string getTypeNameOfInterest() const override;

  std::set<int> getConsumerParamIdx(const std::string &F) const override;

  std::set<int> getFactoryParamIdx(const std::string &F) const override;

  std::string stateToString(TypeStateDescription::State S) const override;

  TypeStateDescription::State bottom() const override;

  TypeStateDescription::State top() const override;

  TypeStateDescription::State uninit() const override;

  TypeStateDescription::State start() const override;

  TypeStateDescription::State error() const override;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPMDCTXDESCRIPTION_H_