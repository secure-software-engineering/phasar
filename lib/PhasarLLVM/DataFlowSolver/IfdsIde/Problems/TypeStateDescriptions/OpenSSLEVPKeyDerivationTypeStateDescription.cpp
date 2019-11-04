/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKeyDerivationTypeStateDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLEVPKeyDerivationTypeStateDescription::OpenSSLEVPKeyDerivationFuncs =
        {{"fopen", {-1}},   {"fdopen", {-1}},   {"fclose", {0}},
         {"fread", {3}},    {"fwrite", {3}},    {"fgetc", {0}},
         {"fgetwc", {0}},   {"fgets", {2}},     {"getc", {0}},
         {"getwc", {0}},    {"_IO_getc", {0}},  {"ungetc", {1}},
         {"ungetwc", {1}},  {"fputc", {1}},     {"fputwc", {1}},
         {"fputs", {1}},    {"putc", {1}},      {"putwc", {1}},
         {"_IO_putc", {1}}, {"fprintf", {0}},   {"fwprintf", {0}},
         {"vfprintf", {0}}, {"vfwprintf", {0}}, {"__isoc99_fscanf", {0}},
         {"fscanf", {0}},   {"fwscanf", {0}},   {"vfscanf", {0}},
         {"vfwscanf", {0}}, {"fflush", {0}},    {"fseek", {0}},
         {"ftell", {0}},    {"rewind", {0}},    {"fgetpos", {0}},
         {"fsetpos", {0}},  {"fileno", {0}}};

// delta[Token][State] = next State
// Token: EVP_KDF_FETCH = 0, EVP_KDF_CTX_NEW = 1, EVP_KDF_CTX_SET_PARAMS = 2,
// DERIVE = 3, EVP_KDF_CTX_FREE = 4, STAR = 5 States: UNINIT = 0, KDF_FETCHED =
// 1, CTX_ATTACHED = 2, PARAM_INIT = 3, DERIVED = 4, ERROR = 5, BOT = 6
const OpenSSLEVPKeyDerivationTypeStateDescription::OpenSSLEVPKeyDerivationState
    OpenSSLEVPKeyDerivationTypeStateDescription::delta[6][7] = {
        /* EVP_KDF_FETCH */
        {OpenSSLEVPKeyDerivationState::KDF_FETCHED,
         OpenSSLEVPKeyDerivationState::KDF_FETCHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
        /* EVP_KDF_CTX_NEW */
        {OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::CTX_ATTACHED,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
        /* EVP_KDF_CTX_SET_PARAMS */
        {OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::PARAM_INIT,
         OpenSSLEVPKeyDerivationState::PARAM_INIT,
         OpenSSLEVPKeyDerivationState::PARAM_INIT,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
        /* DERIVE */
        {OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::DERIVED,
         OpenSSLEVPKeyDerivationState::DERIVED,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
        /* EVP_KDF_CTX_FREE */
        {OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::UNINIT,
         OpenSSLEVPKeyDerivationState::ERROR, OpenSSLEVPKeyDerivationState::BOT,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
        /* STAR */
        {OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::PARAM_INIT,
         OpenSSLEVPKeyDerivationState::DERIVED,
         OpenSSLEVPKeyDerivationState::ERROR,
         OpenSSLEVPKeyDerivationState::BOT},
};

bool OpenSSLEVPKeyDerivationTypeStateDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKeyDerivationFuncs.at(F).find(-1) !=
           OpenSSLEVPKeyDerivationFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKeyDerivationTypeStateDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKeyDerivationFuncs.at(F).find(-1) ==
           OpenSSLEVPKeyDerivationFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKeyDerivationTypeStateDescription::isAPIFunction(
    const std::string &F) const {
  return OpenSSLEVPKeyDerivationFuncs.find(F) !=
         OpenSSLEVPKeyDerivationFuncs.end();
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::getNextState(
    std::string Tok, TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    return delta
        [static_cast<std::underlying_type_t<OpenSSLEVPKeyDerivationToken>>(
            funcNameToToken(Tok))][S];
  } else {
    return OpenSSLEVPKeyDerivationState::BOT;
  }
}

std::string
OpenSSLEVPKeyDerivationTypeStateDescription::getTypeNameOfInterest() const {
  return "struct._IO_FILE"; // NOT SURE WHAT TO D WITH THIS
}

set<int> OpenSSLEVPKeyDerivationTypeStateDescription::getConsumerParamIdx(
    const std::string &F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLEVPKeyDerivationFuncs.at(F);
  }
  return {};
}

set<int> OpenSSLEVPKeyDerivationTypeStateDescription::getFactoryParamIdx(
    const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

std::string OpenSSLEVPKeyDerivationTypeStateDescription::stateToString(
    TypeStateDescription::State S) const {
  switch (S) {
  case OpenSSLEVPKeyDerivationState::TOP:
    return "TOP";
    break;
  case OpenSSLEVPKeyDerivationState::UNINIT:
    return "UNINIT";
    break;
  case OpenSSLEVPKeyDerivationState::KDF_FETCHED:
    return "KDF_FETCHED";
    break;
  case OpenSSLEVPKeyDerivationState::CTX_ATTACHED:
    return "CTX_ATTACHED";
    break;
  case OpenSSLEVPKeyDerivationState::PARAM_INIT:
    return "PARAM_INIT";
    break;
  case OpenSSLEVPKeyDerivationState::DERIVED:
    return "DERIVED";
    break;
  case OpenSSLEVPKeyDerivationState::ERROR:
    return "ERROR";
    break;
  case OpenSSLEVPKeyDerivationState::BOT:
    return "BOT";
    break;
  default:
    assert(false && "received unknown state!");
    break;
  }
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::bottom() const {
  return OpenSSLEVPKeyDerivationState::BOT;
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::top() const {
  return OpenSSLEVPKeyDerivationState::TOP;
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::uninit() const {
  return OpenSSLEVPKeyDerivationState::UNINIT;
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::start() const {
  return OpenSSLEVPKeyDerivationState::KDF_FETCHED;
}

TypeStateDescription::State
OpenSSLEVPKeyDerivationTypeStateDescription::error() const {
  return OpenSSLEVPKeyDerivationState::ERROR;
}

OpenSSLEVPKeyDerivationTypeStateDescription::OpenSSLEVPKeyDerivationToken
OpenSSLEVPKeyDerivationTypeStateDescription::funcNameToToken(
    const std::string &F) const {
  if (F == "EVP_KDF_fetch")
    return OpenSSLEVPKeyDerivationToken::EVP_KDF_FETCH;
  else if (F == "EVP_KDF_CTX_new")
    return OpenSSLEVPKeyDerivationToken::EVP_KDF_CTX_NEW;
  else if (F == "EVP_KDF_CTX_set_params")
    return OpenSSLEVPKeyDerivationToken::EVP_KDF_CTX_SET_PARAMS;
  else if (F == "derive")
    return OpenSSLEVPKeyDerivationToken::DERIVE;
  else if (F == "EVP_KDF_CTX_free")
    return OpenSSLEVPKeyDerivationToken::EVP_KDF_CTX_FREE;
  else
    return OpenSSLEVPKeyDerivationToken::STAR;
}

} // namespace psr
