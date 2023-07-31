/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/ErrorHandling.h"

#include <string>

namespace psr {

namespace {
/**
 * We use the following lattice
 *                BOT = all information
 *
 * UNINIT   OPENED   CLOSED   ERROR
 *
 *                TOP = no information
 */
enum CSTDFILEIOState {
  TOP = 42,
  UNINIT = 0,
  OPENED = 1,
  CLOSED = 2,
  ERROR = 3,
  BOT = 4
};

/**
 * The STAR token represents all API functions besides fopen(), fdopen() and
 * fclose(). FOPEN covers fopen() and fdopen() since both functions are
 * modeled the same in our case.
 */
enum class CSTDFILEIOToken { FOPEN = 0, FCLOSE = 1, STAR = 2 };

constexpr CSTDFILEIOState Delta[3][5] = {
    /* FOPEN */
    {CSTDFILEIOState::OPENED, CSTDFILEIOState::OPENED, CSTDFILEIOState::OPENED,
     CSTDFILEIOState::ERROR, CSTDFILEIOState::OPENED},
    /* FCLOSE */
    {CSTDFILEIOState::ERROR, CSTDFILEIOState::CLOSED, CSTDFILEIOState::ERROR,
     CSTDFILEIOState::ERROR, CSTDFILEIOState::BOT},
    /* STAR */
    {CSTDFILEIOState::ERROR, CSTDFILEIOState::OPENED, CSTDFILEIOState::ERROR,
     CSTDFILEIOState::ERROR, CSTDFILEIOState::BOT},
};

const llvm::StringMap<std::set<int>> &getStdFileIOFuncs() noexcept {
  // Return value is modeled as -1
  static const llvm::StringMap<std::set<int>> StdFileIOFuncs = {
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
  return StdFileIOFuncs;
}

CSTDFILEIOToken funcNameToToken(const std::string &F) {
  if (F == "fopen" || F == "fdopen") {
    return CSTDFILEIOToken::FOPEN;
  }
  if (F == "fclose") {
    return CSTDFILEIOToken::FCLOSE;
  }
  return CSTDFILEIOToken::STAR;
}

} // namespace

// delta[Token][State] = next State
// Token: FOPEN = 0, FCLOSE = 1, STAR = 2
// States: UNINIT = 0, OPENED = 1, CLOSED = 2, ERROR = 3, BOT = 4

bool CSTDFILEIOTypeStateDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return getStdFileIOFuncs().lookup(F).count(-1);
  }
  return false;
}

bool CSTDFILEIOTypeStateDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return !getStdFileIOFuncs().lookup(F).count(-1);
  }
  return false;
}

bool CSTDFILEIOTypeStateDescription::isAPIFunction(const std::string &F) const {
  return getStdFileIOFuncs().count(F);
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::getNextState(
    std::string Tok, TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto X = static_cast<std::underlying_type_t<CSTDFILEIOToken>>(
        funcNameToToken(Tok));

    auto Ret = Delta[X][S];
    // if (ret == error()) {
    //  std::cerr << "getNextState(" << Tok << ", " << stateToString(S)
    //            << ") = " << stateToString(Ret) << std::endl;
    // }
    return Ret;
  }
  return CSTDFILEIOState::BOT;
}

std::string CSTDFILEIOTypeStateDescription::getTypeNameOfInterest() const {
  return "struct._IO_FILE";
}

std::set<int> CSTDFILEIOTypeStateDescription::getConsumerParamIdx(
    const std::string &F) const {
  if (isConsumingFunction(F)) {
    return getStdFileIOFuncs().lookup(F);
  }
  return {};
}

std::set<int>
CSTDFILEIOTypeStateDescription::getFactoryParamIdx(const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

auto CSTDFILEIOTypeStateDescription::getStateToString() const
    -> std::string (*)(int) {
  return [](TypeStateDescription::State S) -> std::string {
    switch (S) {
    case CSTDFILEIOState::TOP:
      return "TOP";
      break;
    case CSTDFILEIOState::UNINIT:
      return "UNINIT";
      break;
    case CSTDFILEIOState::OPENED:
      return "OPENED";
      break;
    case CSTDFILEIOState::CLOSED:
      return "CLOSED";
      break;
    case CSTDFILEIOState::ERROR:
      return "ERROR";
      break;
    case CSTDFILEIOState::BOT:
      return "BOT";
      break;
    default:
      llvm::report_fatal_error("received unknown state!");
      break;
    }
  };
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::bottom() const {
  return CSTDFILEIOState::BOT;
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::top() const {
  return CSTDFILEIOState::TOP;
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::uninit() const {
  return CSTDFILEIOState::UNINIT;
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::start() const {
  return CSTDFILEIOState::OPENED;
}

TypeStateDescription::State CSTDFILEIOTypeStateDescription::error() const {
  return CSTDFILEIOState::ERROR;
}

} // namespace psr
