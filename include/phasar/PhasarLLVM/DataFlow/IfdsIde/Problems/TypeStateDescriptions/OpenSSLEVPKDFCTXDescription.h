/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All righ reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFCTXDESCRIPTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_TYPESTATEDESCRIPTIONS_OPENSSLEVPKDFCTXDESCRIPTION_H

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

#include <map>
#include <set>
#include <string>

namespace llvm {
class Instruction;
class Value;
} // namespace llvm

namespace psr {

/**
 * We use the following lattice
 *                BOT = all information
 *
 * UNINIT     CTX_ATTACHED   PARAM_INIT   DERIVED   ERROR
 *
 *                TOP = no information
 */
enum class OpenSSLEVPKDFCTXState {
  TOP = 42,
  UNINIT = 5,
  CTX_ATTACHED = 1,
  PARAM_INIT = 2,
  DERIVED = 3,
  ERROR = 4,
  BOT = 0 // It is VERY IMPORTANT, athat BOT has value 0, since this is the
          // default value
};

llvm::StringRef to_string(OpenSSLEVPKDFCTXState State) noexcept;

/**
 * A type state description for OpenSSL's EVP Key Derivation functions. The
 * finite state machine is encoded by a two-dimensional array with rows as
 * function tokens and columns as states.
 */
class OpenSSLEVPKDFCTXDescription
    : public TypeStateDescription<OpenSSLEVPKDFCTXState> {
private:
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

  // Delta matrix to implement the state machine's Delta function
  static const OpenSSLEVPKDFCTXState Delta[5][6];

  // std::map<std::pair<const llvm::Instruction *, const llvm::Value *>, int>
  //     requiredKDFState;
  IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFDescription>>
      &KDFAnalysisResults;
  static OpenSSLEVTKDFToken funcNameToToken(llvm::StringRef F);

public:
  using TypeStateDescription::getNextState;
  OpenSSLEVPKDFCTXDescription(
      IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFDescription>>
          &KDFAnalysisResults)
      : KDFAnalysisResults(KDFAnalysisResults) {}

  [[nodiscard]] bool isFactoryFunction(llvm::StringRef FuncName) const override;
  [[nodiscard]] bool
  isConsumingFunction(llvm::StringRef FuncName) const override;
  [[nodiscard]] bool isAPIFunction(llvm::StringRef FuncName) const override;
  [[nodiscard]] State getNextState(llvm::StringRef Tok, State S) const override;
  [[nodiscard]] State
  getNextState(llvm::StringRef Tok, State S,
               const llvm::CallBase *CallSite) const override;
  [[nodiscard]] std::string getTypeNameOfInterest() const override;
  [[nodiscard]] std::set<int>
  getConsumerParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] std::set<int>
  getFactoryParamIdx(llvm::StringRef F) const override;
  [[nodiscard]] State bottom() const override;
  [[nodiscard]] State top() const override;
  [[nodiscard]] State uninit() const override;
  [[nodiscard]] State start() const override;
  [[nodiscard]] State error() const override;
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
