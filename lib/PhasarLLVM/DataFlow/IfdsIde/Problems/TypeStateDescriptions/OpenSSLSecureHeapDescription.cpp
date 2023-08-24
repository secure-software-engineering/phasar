/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureHeapDescription.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
static const std::map<llvm::StringRef, std::set<int>> OpenSSLSecureHeapFuncs = {
    {"CRYPTO_secure_malloc", {-1}},
    {"CRYPTO_secure_zalloc", {-1}},
    {"CRYPTO_secure_free", {0}},
    {"CRYPTO_secure_clear_free", {0}}};

// delta[Token][State] = next State
// Token:  SECURE_MALLOC = 0, SECURE_ZALLOC = 1,  SECURE_FREE = 2,
// SECURE_CLEAR_FREE = 3, STAR = 4
//
// States: BOT = 0, UNINIT = 1, ALLOCATED = 2, ZEROED = 3, FREED = 4, ERROR = 5
const OpenSSLSecureHeapState OpenSSLSecureHeapDescription::Delta[5][6] = {
    // SECURE_MALLOC
    {OpenSSLSecureHeapState::ALLOCATED, OpenSSLSecureHeapState::ALLOCATED,
     OpenSSLSecureHeapState::ALLOCATED, OpenSSLSecureHeapState::ALLOCATED,
     OpenSSLSecureHeapState::ALLOCATED, OpenSSLSecureHeapState::ALLOCATED},
    // SECURE_ZALLOC
    {OpenSSLSecureHeapState::ZEROED, OpenSSLSecureHeapState::ZEROED,
     OpenSSLSecureHeapState::ZEROED, OpenSSLSecureHeapState::ZEROED,
     OpenSSLSecureHeapState::ZEROED, OpenSSLSecureHeapState::ZEROED},
    // SECURE_FREE
    {OpenSSLSecureHeapState::ERROR, OpenSSLSecureHeapState::ERROR,
     OpenSSLSecureHeapState::ERROR, OpenSSLSecureHeapState::FREED,
     OpenSSLSecureHeapState::ERROR, OpenSSLSecureHeapState::ERROR},
    // SECURE_CLEAR_FREE
    {OpenSSLSecureHeapState::ERROR, OpenSSLSecureHeapState::ERROR,
     OpenSSLSecureHeapState::FREED, OpenSSLSecureHeapState::FREED,
     OpenSSLSecureHeapState::ERROR, OpenSSLSecureHeapState::ERROR},
    // STAR
    {OpenSSLSecureHeapState::BOT, OpenSSLSecureHeapState::UNINIT,
     OpenSSLSecureHeapState::ALLOCATED, OpenSSLSecureHeapState::ZEROED,
     OpenSSLSecureHeapState::FREED, OpenSSLSecureHeapState::ERROR},
};
OpenSSLSecureHeapDescription::OpenSSLSecureHeapDescription(
    IDESolver<IDESecureHeapPropagationAnalysisDomain>
        &SecureHeapPropagationResults)
    : SecureHeapPropagationResults(SecureHeapPropagationResults) {}

bool OpenSSLSecureHeapDescription::isFactoryFunction(llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F).find(-1) !=
           OpenSSLSecureHeapFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureHeapDescription::isConsumingFunction(
    llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F).find(-1) ==
           OpenSSLSecureHeapFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLSecureHeapDescription::isAPIFunction(llvm::StringRef F) const {
  return OpenSSLSecureHeapFuncs.find(F) != OpenSSLSecureHeapFuncs.end();
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::getNextState(
    llvm::StringRef Tok, TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto Ftok = static_cast<std::underlying_type_t<OpenSSLSecureHeapToken>>(
        funcNameToToken(Tok));

    return Delta[Ftok][int(S)];
  }
  return OpenSSLSecureHeapState::BOT;
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::getNextState(
    llvm::StringRef Tok, TypeStateDescription::State S,
    const llvm::CallBase *CallSite) const {
  if (isAPIFunction(Tok)) {
    auto Ftok = static_cast<std::underlying_type_t<OpenSSLSecureHeapToken>>(
        funcNameToToken(Tok));
    auto Results = SecureHeapPropagationResults.resultAt(
        CallSite, SecureHeapFact::INITIALIZED);
    if (Results != SecureHeapValue::INITIALIZED) {
      // std::cerr << "ERROR: SecureHeap not initialized at "
      //          << llvmIRToShortString(CS.getInstruction()) << std::endl;
      return error();
    }
    return Delta[Ftok][int(S)];
  }
  return error();
}

std::string OpenSSLSecureHeapDescription::getTypeNameOfInterest() const {
  return "i8";
}

set<int>
OpenSSLSecureHeapDescription::getConsumerParamIdx(llvm::StringRef F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLSecureHeapFuncs.at(F);
  }
  return {};
}

set<int>
OpenSSLSecureHeapDescription::getFactoryParamIdx(llvm::StringRef F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

llvm::StringRef to_string(OpenSSLSecureHeapState State) noexcept {
  switch (State) {
  case OpenSSLSecureHeapState::TOP:
    return "TOP";
  case OpenSSLSecureHeapState::BOT:
    return "BOT";
  case OpenSSLSecureHeapState::ALLOCATED:
    return "ALLOCATED";
  case OpenSSLSecureHeapState::UNINIT:
    return "UNINIT";
  case OpenSSLSecureHeapState::FREED:
    return "FREED";
  case OpenSSLSecureHeapState::ERROR:
    return "ERROR";
  case OpenSSLSecureHeapState::ZEROED:
    return "ZEROED";
  }
  llvm::report_fatal_error("received unknown state!");
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::bottom() const {
  return OpenSSLSecureHeapState::BOT;
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::top() const {
  return OpenSSLSecureHeapState::TOP;
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::start() const {
  llvm::report_fatal_error("TypeStateDescription::start() is deprecated");
  return OpenSSLSecureHeapState::BOT;
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::uninit() const {
  return OpenSSLSecureHeapState::UNINIT;
}

OpenSSLSecureHeapState OpenSSLSecureHeapDescription::error() const {
  return OpenSSLSecureHeapState::ERROR;
}

OpenSSLSecureHeapDescription::OpenSSLSecureHeapToken
OpenSSLSecureHeapDescription::funcNameToToken(llvm::StringRef F) {
  return llvm::StringSwitch<OpenSSLSecureHeapToken>(F)
      .Case("CRYPTO_secure_malloc", OpenSSLSecureHeapToken::SECURE_MALLOC)
      .Case("CRYPTO_secure_zalloc", OpenSSLSecureHeapToken::SECURE_ZALLOC)
      .Case("CRYPTO_secure_free", OpenSSLSecureHeapToken::SECURE_FREE)
      .Case("CRYPTO_secure_clear_free",
            OpenSSLSecureHeapToken::SECURE_CLEAR_FREE)
      .Default(OpenSSLSecureHeapToken::STAR);
}

} // namespace psr
