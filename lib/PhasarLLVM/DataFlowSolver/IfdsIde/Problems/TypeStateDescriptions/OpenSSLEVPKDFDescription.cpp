/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <cassert>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h>

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const std::map<std::string, std::set<int>>
    OpenSSLEVPKDFDescription::OpenSSLEVPKDFFuncs = {
        {"fopen", {-1}},   {"fdopen", {-1}},   {"fclose", {0}},
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
const OpenSSLEVPKDFDescription::OpenSSLEVPKDFState
    OpenSSLEVPKDFDescription::delta[6][7] = {
        /* EVP_KDF_FETCH */
        {OpenSSLEVPKDFState::KDF_FETCHED, OpenSSLEVPKDFState::KDF_FETCHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
        /* EVP_KDF_CTX_NEW */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
        /* EVP_KDF_CTX_SET_PARAMS */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
        /* DERIVE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::DERIVED,
         OpenSSLEVPKDFState::DERIVED, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
        /* EVP_KDF_CTX_FREE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::UNINIT, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
        /* STAR */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::DERIVED, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::BOT},
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
    return delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        funcNameToToken(Tok))][S];
  } else {
    return OpenSSLEVPKDFState::BOT;
  }
}

std::string OpenSSLEVPKDFDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdp_ctx_st"; // NOT SURE WHAT TO DO WITH THIS
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
  case OpenSSLEVPKDFState::KDF_FETCHED:
    return "KDF_FETCHED";
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
  return OpenSSLEVPKDFState::KDF_FETCHED;
}

TypeStateDescription::State OpenSSLEVPKDFDescription::error() const {
  return OpenSSLEVPKDFState::ERROR;
}

OpenSSLEVPKDFDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFDescription::funcNameToToken(const std::string &F) const {
  if (F == "EVP_KDF_fetch")
    return OpenSSLEVTKDFToken::EVP_KDF_FETCH;
  else if (F == "EVP_KDF_CTX_new")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW;
  else if (F == "EVP_KDF_CTX_set_params")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_SET_PARAMS;
  else if (F == "derive")
    return OpenSSLEVTKDFToken::DERIVE;
  else if (F == "EVP_KDF_CTX_free")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_FREE;
  else
    return OpenSSLEVTKDFToken::STAR;
}

} // namespace psr
