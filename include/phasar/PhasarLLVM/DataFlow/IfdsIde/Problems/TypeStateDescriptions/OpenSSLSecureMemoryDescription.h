/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLSECUREMEMORYDESCRIPTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

#include <map>
#include <set>
#include <string>

namespace psr {

class OpenSSLSecureMemoryDescription : public TypeStateDescription {
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
  [[nodiscard]] auto getStateToString() const -> std::string (*)(int) override;
  [[nodiscard]] TypeStateDescription::State bottom() const override;
  [[nodiscard]] TypeStateDescription::State top() const override;
  [[nodiscard]] TypeStateDescription::State uninit() const override;
  [[nodiscard]] TypeStateDescription::State start() const override;
  [[nodiscard]] TypeStateDescription::State error() const override;
};

} // namespace psr

#endif
