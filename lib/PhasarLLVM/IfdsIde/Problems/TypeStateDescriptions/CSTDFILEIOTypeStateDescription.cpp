/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>

#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h>

using namespace std;
using namespace psr;

namespace psr {

const std::map<std::string, int>
    CSTDFILEIOTypeStateDescription::StdFileIOFuncs = {
        {"fopen", -1},    {"fclose", 0},   {"freopen", -1},
        {"fread", 3},     {"fwrite", 3},   {"fgetc", 13},
        {"fputc", 13},    {"putchar", 13}, {"_IO_getc", 13},
        {"_I0_putc", 13}, {"fprintf", 13}, {"__isoc99_fscanf", 13},
        {"feof", 13},     {"ferror", 13},  {"ungetc", 13},
        {"fflush", 13},   {"fseek", 13},   {"ftell", 13},
        {"rewind", 13},   {"fgetpos", 13}, {"fsetpos", 13}};

const CSTDFILEIOState delta[4][4] = {
    {CSTDFILEIOState::OPENED, CSTDFILEIOState::ERROR, CSTDFILEIOState::OPENED,
     CSTDFILEIOState::ERROR},
    {CSTDFILEIOState::UNINIT, CSTDFILEIOState::CLOSED, CSTDFILEIOState::ERROR,
     CSTDFILEIOState::ERROR},
    {CSTDFILEIOState::ERROR, CSTDFILEIOState::OPENED, CSTDFILEIOState::ERROR,
     CSTDFILEIOState::ERROR},
    {CSTDFILEIOState::OPENED, CSTDFILEIOState::OPENED, CSTDFILEIOState::OPENED,
     CSTDFILEIOState::ERROR},
};

bool CSTDFILEIOTypeStateDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return StdFileIOFuncs.at(F) == -1;
  }
  return false;
}

bool CSTDFILEIOTypeStateDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return StdFileIOFuncs.at(F) != -1;
  }
  return false;
}

bool CSTDFILEIOTypeStateDescription::isAPIFunction(const std::string &F) const {
  return StdFileIOFuncs.count(F);
}

CSTDFILEIOState
CSTDFILEIOTypeStateDescription::getNextState(std::string Tok,
                                             CSTDFILEIOState S) {
  // return delta[static_cast<std::underlying_type_t<Token>>(tok)]
  // [static_cast<std::underlying_type_t<State>>(state)];
  return CSTDFILEIOState::BOT;
}

std::string CSTDFILEIOTypeStateDescription::getTypeNameOfInterest() const {
  return "struct._IO_FILE";
}

int CSTDFILEIOTypeStateDescription::getConsumerParamIdx(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return StdFileIOFuncs.at(F);
  }
  return -1;
}

std::string
CSTDFILEIOTypeStateDescription::stateToString(CSTDFILEIOState State) const {
  switch (State) {
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
    assert(false && "received unknown state!");
    break;
  }
}

CSTDFILEIOState CSTDFILEIOTypeStateDescription::bottom() const {
  return CSTDFILEIOState::BOT;
}

CSTDFILEIOState CSTDFILEIOTypeStateDescription::top() const {
  return CSTDFILEIOState::TOP;
}

CSTDFILEIOState CSTDFILEIOTypeStateDescription::uninit() const {
  return CSTDFILEIOState::UNINIT;
}

} // namespace psr
