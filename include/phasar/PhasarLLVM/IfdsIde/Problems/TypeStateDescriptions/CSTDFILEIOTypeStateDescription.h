/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_CSTDFILEIOTYPESTATEDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_CSTDFILEIOTYPESTATEDESCRIPTION_H_

#include <map>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>

namespace psr {

enum class CSTDFILEIOState {
  TOP = 42,
  UNINIT = 0,
  OPENED = 1,
  CLOSED = 2,
  ERROR = 3,
  BOT = 13
};

class CSTDFILEIOTypeStateDescription : TypeStateDescription<CSTDFILEIOState> {
private:
  static const std::map<std::string, int> StdFileIOFuncs;
  static const CSTDFILEIOState delta[4][4];

public:
  bool isFactoryFunction(const std::string &F) const override;
  bool isConsumingFunction(const std::string &F) const override;
  bool isAPIFunction(const std::string &F) const override;
  CSTDFILEIOState getNextState(std::string Tok, CSTDFILEIOState S) override;
  std::string getTypeNameOfInterest() const override;
  int getConsumerParamIdx(const std::string &F) const override;
  std::string stateToString(CSTDFILEIOState) const override;
  CSTDFILEIOState bottom() const override;
  CSTDFILEIOState top() const override;
  CSTDFILEIOState uninit() const override;
};

} // namespace psr

#endif
