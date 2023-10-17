/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include <string>

using namespace std;
using namespace psr;

namespace psr {
enum class OpenSSLSecureMemoryState {
  TOP = 42,
  BOT = 0,
  ZEROED = 1,
  FREED = 2,
  ERROR = 3,
  ALLOCATED = 4
};

namespace {

enum class OpenSSLSecureMemoryToken {
  CRYPTO_MALLOC = 0,
  CRYPTO_ZALLOC = 1,
  CRYPTO_FREE = 2,
  OPENSSL_CLEANSE = 3,
  STAR = 4
};

constexpr std::array<llvm::StringLiteral, 2> FactoryFuncs = {"CRYPTO_malloc",
                                                             "CRYPTO_zalloc"};
constexpr std::array<std::pair<llvm::StringLiteral, int>, 2> ConsumingFuncs = {
    {{"CRYPTO_free", 0}, {"OPENSSL_cleanse", 0}},
};

// delta[Token][State] = next State
// Token: CRYPTO_MALLOC=0, CRYPTO_ZALLOC=1, CRYPTO_FREE=2, OPENSSL_CLEANSE=3,
// STAR = 4
//
// States: ALLOCATED=4, ZEROED=1, FREED=2, ERROR=3, BOT=0
constexpr OpenSSLSecureMemoryState Delta[6][7] = {
    // CRYPTO_malloc
    {OpenSSLSecureMemoryState::ALLOCATED, OpenSSLSecureMemoryState::ALLOCATED,
     OpenSSLSecureMemoryState::ALLOCATED, OpenSSLSecureMemoryState::ERROR,
     OpenSSLSecureMemoryState::ALLOCATED},
    // CRYPTO_ZALLOC
    {OpenSSLSecureMemoryState::ZEROED, OpenSSLSecureMemoryState::ZEROED,
     OpenSSLSecureMemoryState::ZEROED, OpenSSLSecureMemoryState::ERROR,
     OpenSSLSecureMemoryState::ZEROED},
    // CRYPTO_FREE
    {OpenSSLSecureMemoryState::ERROR, OpenSSLSecureMemoryState::FREED,
     OpenSSLSecureMemoryState::ERROR, OpenSSLSecureMemoryState::ERROR,
     OpenSSLSecureMemoryState::ERROR},
    // OPENSSL_CLEANSE
    {OpenSSLSecureMemoryState::BOT, OpenSSLSecureMemoryState::ZEROED,
     OpenSSLSecureMemoryState::ERROR, OpenSSLSecureMemoryState::ERROR,
     OpenSSLSecureMemoryState::ZEROED},
    // STAR (kills ZEROED)
    {OpenSSLSecureMemoryState::BOT, OpenSSLSecureMemoryState::ALLOCATED,
     OpenSSLSecureMemoryState::FREED, OpenSSLSecureMemoryState::ERROR,
     OpenSSLSecureMemoryState::ALLOCATED}};

OpenSSLSecureMemoryToken funcNameToToken(llvm::StringRef F) {
  return llvm::StringSwitch<OpenSSLSecureMemoryToken>(F)
      .Case("CRYPTO_malloc", OpenSSLSecureMemoryToken::CRYPTO_MALLOC)
      .Case("CRYPTO_zalloc", OpenSSLSecureMemoryToken::CRYPTO_ZALLOC)
      .Case("CRYPTO_free", OpenSSLSecureMemoryToken::CRYPTO_FREE)
      .Case("OPENSSL_cleanse", OpenSSLSecureMemoryToken::OPENSSL_CLEANSE)
      .Default(OpenSSLSecureMemoryToken::STAR);
}

} // namespace

bool OpenSSLSecureMemoryDescription::isFactoryFunction(
    llvm::StringRef F) const {
  return llvm::is_contained(FactoryFuncs, F);
}

bool OpenSSLSecureMemoryDescription::isConsumingFunction(
    llvm::StringRef F) const {
  return llvm::find_if(ConsumingFuncs, [&F](const auto &Pair) {
           return F == Pair.first;
         }) != ConsumingFuncs.end();
}

bool OpenSSLSecureMemoryDescription::isAPIFunction(llvm::StringRef F) const {
  return funcNameToToken(F) != OpenSSLSecureMemoryToken::STAR;
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::getNextState(
    llvm::StringRef Tok, TypeStateDescription::State S) const {
  auto Token = funcNameToToken(Tok);
  if (Token != OpenSSLSecureMemoryToken::STAR) {
    return Delta[static_cast<std::underlying_type_t<OpenSSLSecureMemoryToken>>(
        Token)][int(S)];
  }
  return OpenSSLSecureMemoryState::BOT;
}

std::string OpenSSLSecureMemoryDescription::getTypeNameOfInterest() const {
  return "i8"; // NOT SURE WHAT TO DO WITH THIS
}

set<int>
OpenSSLSecureMemoryDescription::getConsumerParamIdx(llvm::StringRef F) const {
  if (const auto *It = llvm::find_if(
          ConsumingFuncs, [&F](const auto &Pair) { return F == Pair.first; });
      It != ConsumingFuncs.end()) {
    return {It->second};
  }
  return {};
}

set<int>
OpenSSLSecureMemoryDescription::getFactoryParamIdx(llvm::StringRef F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

llvm::StringRef to_string(OpenSSLSecureMemoryState State) noexcept {
  switch (State) {
  case OpenSSLSecureMemoryState::TOP:
    return "TOP";
  case OpenSSLSecureMemoryState::BOT:
    return "BOT";
  case OpenSSLSecureMemoryState::ALLOCATED:
    return "ALLOCATED";
  case OpenSSLSecureMemoryState::FREED:
    return "FREED";
  case OpenSSLSecureMemoryState::ZEROED:
    return "ZEROED";
  case OpenSSLSecureMemoryState::ERROR:
    return "ERROR";
  }
  llvm::report_fatal_error("received unknown state!");
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::bottom() const {
  return OpenSSLSecureMemoryState::BOT;
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::top() const {
  return OpenSSLSecureMemoryState::TOP;
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::start() const {
  return OpenSSLSecureMemoryState::ALLOCATED;
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::uninit() const {
  return OpenSSLSecureMemoryState::BOT;
}

OpenSSLSecureMemoryState OpenSSLSecureMemoryDescription::error() const {
  return OpenSSLSecureMemoryState::ERROR;
}

} // namespace psr
