/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All righ reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFDESCRIPTION_H

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

namespace psr {
class OpenSSLEVPKDFDescription : public TypeStateDescription {
public:
  /**
   * We use the following lattice
   *                BOT = all information
   *
   *         UNINIT     KDF_FETCHED     ERROR
   *
   *                TOP = no information
   */
  enum OpenSSLEVPKDFState {
    TOP = 42,
    UNINIT = 0,
    KDF_FETCHED = 1,
    ERROR = 2,
    BOT = 3
  };

  /**
   * The STAR token represents all functions besides EVP_KDF_fetch(),
   * EVP_KDF_fetch()  and EVP_KDF_CTX_free().
   */
  enum class OpenSSLEVTKDFToken {
    EVP_KDF_FETCH = 0,
    EVP_KDF_FREE = 1,
    STAR = 2
  };

private:
  static const std::map<std::string, std::set<int>> OpenSSLEVPKDFFuncs;
  // delta matrix to implement the state machine's delta function
  static const OpenSSLEVPKDFState Delta[3][4];
  static OpenSSLEVTKDFToken funcNameToToken(const std::string &F);

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
