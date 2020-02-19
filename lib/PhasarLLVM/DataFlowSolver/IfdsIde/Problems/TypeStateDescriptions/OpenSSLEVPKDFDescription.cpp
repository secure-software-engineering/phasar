/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <cassert>

#include <iostream>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLEVPKDFDescription::OpenSSLEVPKDFFuncs = {
        {"EVP_KDF_fetch", {-1}},
        {"EVP_KDF_CTX_new", {-1}},
        {"EVP_KDF_CTX_set_params", {0}},
        {"EVP_KDF_derive", {0}},
        {"EVP_KDF_CTX_free", {0}}

};

// delta[Token][State] = next State
// Token: EVP_KDF_CTX_NEW = 0,
// EVP_KDF_CTX_SET_PARAMS = 1,
// DERIVE = 2,
// EVP_KDF_CTX_FREE = 3,
// STAR = 4
//
// States: UNINIT = 0, CTX_ATTACHED =1, PARAM_INIT = 2,
// DERIVED = 3, ERROR = 4, BOT = 5
const OpenSSLEVPKDFDescription::OpenSSLEVPKDFState
    OpenSSLEVPKDFDescription::delta[5][6] = {

        /* EVP_KDF_CTX_NEW */
        {OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::CTX_ATTACHED},
        /* EVP_KDF_CTX_SET_PARAMS */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::BOT},
        /* DERIVE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::DERIVED, OpenSSLEVPKDFState::DERIVED,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::BOT},
        /* EVP_KDF_CTX_FREE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::UNINIT,
         OpenSSLEVPKDFState::UNINIT, OpenSSLEVPKDFState::UNINIT,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::BOT},

        /* STAR */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::DERIVED,
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
  return "struct.evp_kdf_ctx_st";
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
    break;
  case OpenSSLEVPKDFState::UNINIT:
    return "UNINIT";
    break;

  case OpenSSLEVPKDFState::CTX_ATTACHED:
    return "CTX_ATTACHED";
    break;
  case OpenSSLEVPKDFState::PARAM_INIT:
    return "PARAM_INIT";
    break;
  case OpenSSLEVPKDFState::DERIVED:
    return "DERIVED";
    break;
  case OpenSSLEVPKDFState::ERROR:
    return "ERROR";
    break;
  case OpenSSLEVPKDFState::BOT:
    return "BOT";
    break;
  default:
    assert(false && "received unknown state!");
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
  return OpenSSLEVPKDFState::CTX_ATTACHED;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::error() const {
  return OpenSSLEVPKDFState::ERROR;
}

OpenSSLEVPKDFDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFDescription::funcNameToToken(const std::string &F) const {
  if (F == "EVP_KDF_CTX_new")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW;
  else if (F == "EVP_KDF_CTX_set_params")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_SET_PARAMS;
  else if (F == "EVP_KDF_derive")
    return OpenSSLEVTKDFToken::DERIVE;

  else if (F == "EVP_KDF_CTX_free")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_FREE;
  else
    return OpenSSLEVTKDFToken::STAR;
}

} // namespace psr
