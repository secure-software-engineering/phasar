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

#include <llvm/ADT/StringRef.h>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/Utils/EnumFlags.h"

#include <array>

namespace psr {
class OpenSSLEVPCIPHERCTXDescription : public TypeStateDescription {

  enum OpenSSLEVPCIPHERCTXState {
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

  static OpenSSLEVPCIPHERCTXToken funcNameToToken(llvm::StringRef F);

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
#endif