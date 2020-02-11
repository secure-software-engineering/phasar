/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <cassert>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryFuncs = {
        {"CRYPTO_malloc", {-1}}, {"CRYPTO_free", {0}}};

// delta[Token][State] = next State
// Token: CRYPTO_MALLOC=0, CRYPTO_FREE=1, STAR = 2
//
// States: ALLOCATED=0, FREED=1, ERROR=2, BOT=3
const OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryState
    OpenSSLSecureMemoryDescription::delta[6][7] = {
        // CRYPTO_malloc
        {OpenSSLSecureMemoryState::ALLOCATED,
         OpenSSLSecureMemoryState::ALLOCATED, OpenSSLSecureMemoryState::ERROR,
         OpenSSLSecureMemoryState::ALLOCATED},
        // CRYPTO_FREE
        {OpenSSLSecureMemoryState::FREED, OpenSSLSecureMemoryState::ERROR,
         OpenSSLSecureMemoryState::ERROR, OpenSSLSecureMemoryState::ERROR},
        // STAR
        {OpenSSLSecureMemoryState::ALLOCATED, OpenSSLSecureMemoryState::FREED,
         OpenSSLSecureMemoryState::ERROR, OpenSSLSecureMemoryState::BOT}};

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
  return "i8"; // NOT SURE WHAT TO DO WITH THIS
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
  case OpenSSLSecureMemoryState::BOT:
    return "BOT";
  case OpenSSLSecureMemoryState::ALLOCATED:
    return "ALLOCATED";
  case OpenSSLSecureMemoryState::FREED:
    return "FREED";
  case OpenSSLSecureMemoryState::ERROR:
    return "ERROR";
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
  return OpenSSLSecureMemoryState::ALLOCATED;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::uninit() const {
  return OpenSSLSecureMemoryState::BOT;
}

TypeStateDescription::State OpenSSLSecureMemoryDescription::error() const {
  return OpenSSLSecureMemoryState::ERROR;
}

OpenSSLSecureMemoryDescription::OpenSSLSecureMemoryToken
OpenSSLSecureMemoryDescription::funcNameToToken(const std::string &F) const {
  return llvm::StringSwitch<OpenSSLSecureMemoryToken>(F)
      .Case("CRYPTO_malloc", OpenSSLSecureMemoryToken::CRYPTO_MALLOC)
      .Case("CRYPTO_free", OpenSSLSecureMemoryToken::CRYPTO_FREE)
      .Default(OpenSSLSecureMemoryToken::STAR);
}

} // namespace psr
