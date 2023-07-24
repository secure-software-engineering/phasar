/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_CSTDFILEIOTYPESTATEDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_CSTDFILEIOTYPESTATEDESCRIPTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include <map>
#include <set>
#include <string>

namespace psr {

/**
 * A type state description for C's file I/O API. The finite state machine
 * is encoded by a two-dimensional array with rows as function tokens and
 * columns as states.
 */
class CSTDFILEIOTypeStateDescription : public TypeStateDescription {
private:
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

  static const std::map<std::string, std::set<int>> StdFileIOFuncs;
  // delta matrix to implement the state machine's delta function
  static const CSTDFILEIOState Delta[3][5];
  static CSTDFILEIOToken funcNameToToken(const std::string &F);

public:
  [[nodiscard]] bool isFactoryFunction(const std::string &F) const override;
  [[nodiscard]] bool isConsumingFunction(const std::string &F) const override;
  [[nodiscard]] bool isAPIFunction(const std::string &F) const override;
  [[nodiscard]] TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
  [[nodiscard]] std::string getTypeNameOfInterest() const override;
  [[nodiscard]] std::set<int>
  getConsumerParamIdx(const std::string &F) const override;
  [[nodiscard]] std::set<int>
  getFactoryParamIdx(const std::string &F) const override;
  [[nodiscard]] std::string
  stateToString(TypeStateDescription::State S) const override;
  [[nodiscard]] TypeStateDescription::State bottom() const override;
  [[nodiscard]] TypeStateDescription::State top() const override;
  [[nodiscard]] TypeStateDescription::State uninit() const override;
  [[nodiscard]] TypeStateDescription::State start() const override;
  [[nodiscard]] TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
