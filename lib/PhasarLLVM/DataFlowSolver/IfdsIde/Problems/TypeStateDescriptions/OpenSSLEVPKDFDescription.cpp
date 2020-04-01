/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>
#include <map>

#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLEVPKDFDescription::OpenSSLEVPKDFFuncs = {{"EVP_KDF_fetch", {-1}},
                                                    {"EVP_KDF_free", {0}}

};

// delta[Token][State] = next State
// Token: EVP_KDF_FETCH = 0,
// EVP_KDF_FREE = 1,
// STAR = 2
//
// States: UNINIT = 0, KDF_FETCHED = 1, ERROR = 2, BOT = 3
const OpenSSLEVPKDFDescription::OpenSSLEVPKDFState
    OpenSSLEVPKDFDescription::delta[3][4] = {
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

bool OpenSSLEVPKDFDescription::isFactoryFunction(const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) != OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFDescription::isConsumingFunction(const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) == OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFDescription::isAPIFunction(const std::string &F) const {
  return OpenSSLEVPKDFFuncs.find(F) != OpenSSLEVPKDFFuncs.end();
}

TypeStateDescription::State
OpenSSLEVPKDFDescription::getNextState(std::string Tok,
                                       TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto ret = delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        funcNameToToken(Tok))][S];
    // std::cout << "delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return ret;
  } else {
    return OpenSSLEVPKDFState::BOT;
  }
}

std::string OpenSSLEVPKDFDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdf_st";
}

set<int>
OpenSSLEVPKDFDescription::getConsumerParamIdx(const std::string &F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F);
  }
  return {};
}

set<int>
OpenSSLEVPKDFDescription::getFactoryParamIdx(const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

std::string
OpenSSLEVPKDFDescription::stateToString(TypeStateDescription::State S) const {
  switch (S) {
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
  default:
    llvm::report_fatal_error("received unknown state!");
    break;
  }
}

TypeStateDescription::State OpenSSLEVPKDFDescription::bottom() const {
  return OpenSSLEVPKDFState::BOT;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::top() const {
  return OpenSSLEVPKDFState::TOP;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::uninit() const {
  return OpenSSLEVPKDFState::UNINIT;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::start() const {
  return OpenSSLEVPKDFState::KDF_FETCHED;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::error() const {
  return OpenSSLEVPKDFState::ERROR;
}

OpenSSLEVPKDFDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFDescription::funcNameToToken(const std::string &F) const {
  if (F == "EVP_KDF_fetch")
    return OpenSSLEVTKDFToken::EVP_KDF_FETCH;
  else if (F == "EVP_KDF_free")
    return OpenSSLEVTKDFToken::EVP_KDF_FREE;
  else
    return OpenSSLEVTKDFToken::STAR;
}

} // namespace psr
