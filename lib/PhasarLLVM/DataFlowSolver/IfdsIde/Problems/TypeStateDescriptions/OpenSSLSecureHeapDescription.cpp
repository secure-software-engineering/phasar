/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <cassert>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureHeapDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLSecureHeapDescription::OpenSSLSecureHeapFuncs = {
        // TODO
};

// delta[Token][State] = next State
// Token: EVP_KDF_FETCH = 0, EVP_KDF_CTX_NEW = 1, EVP_KDF_CTX_SET_PARAMS = 2,
// DERIVE = 3, EVP_KDF_CTX_FREE = 4, STAR = 5 States: UNINIT = 0, KDF_FETCHED =
// 1, CTX_ATTACHED = 2, PARAM_INIT = 3, DERIVED = 4, ERROR = 5, BOT = 6
const OpenSSLSecureHeapDescription::OpenSSLSecureHeapState
    OpenSSLSecureHeapDescription::delta[6][7] = {
        // TODO
};

bool OpenSSLSecureHeapDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F).find(-1) !=
           OpenSSLSecureHeapFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureHeapDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F).find(-1) ==
           OpenSSLSecureHeapFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureHeapDescription::isAPIFunction(const std::string &F) const {
  return OpenSSLSecureHeapFuncs.find(F) != OpenSSLSecureHeapFuncs.end();
}

TypeStateDescription::State OpenSSLSecureHeapDescription::getNextState(
    std::string Tok, TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    return delta[static_cast<std::underlying_type_t<OpenSSLSecureHeapToken>>(
        funcNameToToken(Tok))][S];
  } else {
    return OpenSSLSecureHeapState::BOT;
  }
}

std::string OpenSSLSecureHeapDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdp_ctx_st"; // NOT SURE WHAT TO DO WITH THIS
}

set<int>
OpenSSLSecureHeapDescription::getConsumerParamIdx(const std::string &F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F);
  }
  return {};
}

set<int>
OpenSSLSecureHeapDescription::getFactoryParamIdx(const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

std::string OpenSSLSecureHeapDescription::stateToString(
    TypeStateDescription::State S) const {
  switch (S) {
  case OpenSSLSecureHeapState::TOP:
    return "TOP";
    break;
  case OpenSSLSecureHeapState::BOT:
    return "BOT";
    break;
  default:
    assert(false && "received unknown state!");
    break;
  }
}

TypeStateDescription::State OpenSSLSecureHeapDescription::bottom() const {
  return OpenSSLSecureHeapState::BOT;
}

TypeStateDescription::State OpenSSLSecureHeapDescription::top() const {
  return OpenSSLSecureHeapState::TOP;
}

TypeStateDescription::State OpenSSLSecureHeapDescription::start() const {
  return OpenSSLSecureHeapState::BOT;
}

TypeStateDescription::State OpenSSLSecureHeapDescription::uninit() const {
  return OpenSSLSecureHeapState::BOT;
}

TypeStateDescription::State OpenSSLSecureHeapDescription::error() const {
  return OpenSSLSecureHeapState::TOP;
}

OpenSSLSecureHeapDescription::OpenSSLSecureHeapToken
OpenSSLSecureHeapDescription::funcNameToToken(const std::string &F) const {
  return OpenSSLSecureHeapToken::STAR;
}

} // namespace psr
