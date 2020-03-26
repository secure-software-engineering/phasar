/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All righ reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFCTXDESCRIPTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFCTXDESCRIPTION_H_

#include <functional>
#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

namespace llvm {
class Instruction;
class Value;
} // namespace llvm

namespace psr {

/**
 * A type state description for OpenSSL's EVP Key Derivation functions. The
 * finite state machine is encoded by a two-dimensional array with rows as
 * function tokens and columns as states.
 */
class OpenSSLEVPKDFCTXDescription : public TypeStateDescription {
private:
  /**
   * We use the following lattice
   *                BOT = all information
   *
   * UNINIT     CTX_ATTACHED   PARAM_INIT   DERIVED   ERROR
   *
   *                TOP = no information
   */
  enum OpenSSLEVPKDFState {
    TOP = 42,
    UNINIT = 5,
    CTX_ATTACHED = 1,
    PARAM_INIT = 2,
    DERIVED = 3,
    ERROR = 4,
    BOT = 0 // It is VERY IMPORTANT, athat BOT has value 0, since this is the
            // default value
  };

  /**
   * The STAR token represents all functions besides EVP_KDF_fetch(),
   * EVP_KDF_CTX_new(), EVP_KDF_CTX_set_params() ,derive() and
   * EVP_KDF_CTX_free().
   */
  enum class OpenSSLEVTKDFToken {
    EVP_KDF_CTX_NEW = 0,
    EVP_KDF_CTX_SET_PARAMS = 1,
    DERIVE = 2,
    EVP_KDF_CTX_FREE = 3,
    STAR = 4
  };

  static const std::map<std::string, std::set<int>> OpenSSLEVPKDFFuncs;
  // delta matrix to implement the state machine's delta function
  static const OpenSSLEVPKDFState delta[5][6];

  // std::map<std::pair<const llvm::Instruction *, const llvm::Value *>, int>
  //     requiredKDFState;
  IDESolver<const llvm::Instruction *, const llvm::Value *,
            const llvm::Function *, const llvm::StructType *,
            const llvm::Value *, int, LLVMBasedICFG> &kdfAnalysisResults;
  OpenSSLEVTKDFToken funcNameToToken(const std::string &F) const;

public:
  OpenSSLEVPKDFCTXDescription(
      IDESolver<const llvm::Instruction *, const llvm::Value *,
                const llvm::Function *, const llvm::StructType *,
                const llvm::Value *, int, LLVMBasedICFG> &kdfAnalysisResults)
      : kdfAnalysisResults(kdfAnalysisResults) {}
  bool isFactoryFunction(const std::string &F) const override;
  bool isConsumingFunction(const std::string &F) const override;
  bool isAPIFunction(const std::string &F) const override;
  TypeStateDescription::State
  getNextState(std::string Tok, TypeStateDescription::State S) const override;
  TypeStateDescription::State
  getNextState(const std::string &Tok, TypeStateDescription::State S,
               llvm::ImmutableCallSite CS) const override;
  std::string getTypeNameOfInterest() const override;
  std::set<int> getConsumerParamIdx(const std::string &F) const override;
  std::set<int> getFactoryParamIdx(const std::string &F) const override;
  std::string stateToString(TypeStateDescription::State S) const override;
  TypeStateDescription::State bottom() const override;
  TypeStateDescription::State top() const override;
  TypeStateDescription::State uninit() const override;
  TypeStateDescription::State start() const override;
  TypeStateDescription::State error() const override;
  /*
    /// Checks all callSites, where a EVP_KDF object needs to be in a
    /// certain state, such that the state transition for EVP_KDF_CTX is valid.
    /// Concretely, all calls to EVP_KDF_CTX_new are checked. The first
    parameter
    /// needs to be in KDF_FETCHED state for this calls being valid.
    ///
    /// \param onError A callback, that is invoked, whenever the validation
    finds
    /// a callsite, where the expected typestate does not match the actual
    state.
    /// The first two parameters define the exact object and the callSite, where
    /// the error occurred. The first integer denotes the expected typestate and
    /// the second integer the actual typestate.
    bool validateKDFConstraints(
        IDESolver<const llvm::Instruction *, const llvm::Value *,
                  const llvm::Function *, const llvm::StructType *,
                  const llvm::Value *, int, LLVMBasedICFG> &kdfAnalysisResults,
        std::function<void(const llvm::Instruction *, const llvm::Value *, int,
                           int)>
            onError) const;
  */
};

} // namespace psr

#endif
