/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_TYPESTATEDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_TYPESTATEDESCRIPTION_H_

#include <string>

namespace psr {

template <typename State> struct TypeStateDescription {
  virtual ~TypeStateDescription() = default;
  virtual bool isFactoryFunction(const std::string &F) const = 0;
  virtual bool isConsumingFunction(const std::string &F) const = 0;
  virtual bool isAPIFunction(const std::string &F) const = 0;
  virtual State getNextState(std::string Tok, State S) = 0;
  virtual std::string getTypeNameOfInterest() const = 0;
  virtual int getConsumerParamIdx(const std::string &F) const = 0;
  virtual std::string stateToString(State S) const = 0;
  virtual State bottom() const = 0;
  virtual State top() const = 0;
  virtual State uninit() const = 0;
};

} // namespace psr

#endif
