/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <cassert>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryFuncs = {
        // TODO
};

// delta[Token][State] = next State
// Token: EVP_KDF_FETCH = 0, EVP_KDF_CTX_NEW = 1, EVP_KDF_CTX_SET_PARAMS = 2,
// DERIVE = 3, EVP_KDF_CTX_FREE = 4, STAR = 5 States: UNINIT = 0, KDF_FETCHED =
// 1, CTX_ATTACHED = 2, PARAM_INIT = 3, DERIVED = 4, ERROR = 5, BOT = 6
const OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryState
    OpenSSLSecureMemoryDescription::delta[6][7] = {
        // TODO
};

bool OpenSSLSecureMemoryDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureMemoryFuncs.at(F).find(-1) !=
           OpenSSLSecureMemoryFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureMemoryDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureMemoryFuncs.at(F).find(-1) ==
           OpenSSLSecureMemoryFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureMemoryDescription::isAPIFunction(const std::string &F) const {
  return OpenSSLSecureMemoryFuncs.find(F) != OpenSSLSecureMemoryFuncs.end();
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::getNextState(
    std::string Tok, TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    return delta[static_cast<std::underlying_type_t<OpenSSLSecureMemoryToken>>(
        funcNameToToken(Tok))][S];
  } else {
    return OpenSSLSecureMemoryState::BOT;
  }
}

std::string OpenSSLSecureMemoryDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdp_ctx_st"; // NOT SURE WHAT TO DO WITH THIS
}

set<int> OpenSSLSecureMemoryDescription::getConsumerParamIdx(
    const std::string &F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLSecureMemoryFuncs.at(F);
  }
  return {};
}

set<int>
OpenSSLSecureMemoryDescription::getFactoryParamIdx(const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

std::string OpenSSLSecureMemoryDescription::stateToString(
    TypeStateDescription::State S) const {
  switch (S) {
  case OpenSSLSecureMemoryState::TOP:
    return "TOP";
    break;
  case OpenSSLSecureMemoryState::BOT:
    return "BOT";
    break;
  default:
    assert(false && "received unknown state!");
    break;
  }
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::bottom() const {
  return OpenSSLSecureMemoryState::BOT;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::top() const {
  return OpenSSLSecureMemoryState::TOP;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::start() const {
  return OpenSSLSecureMemoryState::BOT;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::uninit() const {
  return OpenSSLSecureMemoryState::BOT;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::error() const {
  return OpenSSLSecureMemoryState::BOT;
}

OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryToken
OpenSSLSecureMemoryDescription::funcNameToToken(const std::string &F) const {
  return OpenSSLSecureMemoryToken::STAR;
}

} // namespace psr
