/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include "llvm/Support/ErrorHandling.h"

#include <map>

namespace psr {

// Return value is modeled as -1
static const std::map<llvm::StringRef, std::set<int>> OpenSSLEVPKDFFuncs = {
    {"EVP_KDF_fetch", {-1}}, {"EVP_KDF_free", {0}}

};

// Delta[Token][State] = next State
// Token: EVP_KDF_FETCH = 0,
// EVP_KDF_FREE = 1,
// STAR = 2
//
// States: UNINIT = 0, KDF_FETCHED = 1, ERROR = 2, BOT = 3
const OpenSSLEVPKDFState OpenSSLEVPKDFDescription::Delta[3][4] = {
    /* EVP_KDF_FETCH */
    {OpenSSLEVPKDFState::KDF_FETCHED, OpenSSLEVPKDFState::ERROR,
     OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::KDF_FETCHED},
    /* EVP_KDF_CTX_FREE */
    {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::UNINIT,
     OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::BOT},

    /* STAR */
    {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::KDF_FETCHED,
     OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::BOT},
};

bool OpenSSLEVPKDFDescription::isFactoryFunction(llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) != OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFDescription::isConsumingFunction(llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) == OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFDescription::isAPIFunction(llvm::StringRef F) const {
  return OpenSSLEVPKDFFuncs.find(F) != OpenSSLEVPKDFFuncs.end();
}

OpenSSLEVPKDFState
OpenSSLEVPKDFDescription::getNextState(llvm::StringRef Tok,
                                       TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto Ret = Delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        funcNameToToken(Tok))][int(S)];
    // std::cout << "Delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return Ret;
  }
  return OpenSSLEVPKDFState::BOT;
}

std::string OpenSSLEVPKDFDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdf_st";
}

std::set<int>
OpenSSLEVPKDFDescription::getConsumerParamIdx(llvm::StringRef F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F);
  }
  return {};
}

std::set<int>
OpenSSLEVPKDFDescription::getFactoryParamIdx(llvm::StringRef F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

llvm::StringRef to_string(OpenSSLEVPKDFState State) noexcept {
  switch (State) {
  case OpenSSLEVPKDFState::TOP:
    return "TOP";
  case OpenSSLEVPKDFState::UNINIT:
    return "UNINIT";
  case OpenSSLEVPKDFState::KDF_FETCHED:
    return "KDF_FETCHED";
  case OpenSSLEVPKDFState::ERROR:
    return "ERROR";
  case OpenSSLEVPKDFState::BOT:
    return "BOT";
  }
  llvm::report_fatal_error("received unknown state!");
}

OpenSSLEVPKDFState OpenSSLEVPKDFDescription::bottom() const {
  return OpenSSLEVPKDFState::BOT;
}

OpenSSLEVPKDFState OpenSSLEVPKDFDescription::top() const {
  return OpenSSLEVPKDFState::TOP;
}

OpenSSLEVPKDFState OpenSSLEVPKDFDescription::uninit() const {
  return OpenSSLEVPKDFState::UNINIT;
}

OpenSSLEVPKDFState OpenSSLEVPKDFDescription::start() const {
  return OpenSSLEVPKDFState::KDF_FETCHED;
}

OpenSSLEVPKDFState OpenSSLEVPKDFDescription::error() const {
  return OpenSSLEVPKDFState::ERROR;
}

OpenSSLEVPKDFDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFDescription::funcNameToToken(llvm::StringRef FuncName) {
  if (FuncName == "EVP_KDF_fetch") {
    return OpenSSLEVTKDFToken::EVP_KDF_FETCH;
  }
  if (FuncName == "EVP_KDF_free") {
    return OpenSSLEVTKDFToken::EVP_KDF_FREE;
  }
  return OpenSSLEVTKDFToken::STAR;
}

} // namespace psr
