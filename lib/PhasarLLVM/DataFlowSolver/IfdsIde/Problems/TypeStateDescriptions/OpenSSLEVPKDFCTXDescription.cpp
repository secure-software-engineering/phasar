/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>
#include <set>
#include <string>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

using namespace std;
using namespace psr;

namespace psr {

// Return value is modeled as -1
const map<string, set<int>> OpenSSLEVPKDFCTXDescription::OpenSSLEVPKDFFuncs = {
    {"EVP_KDF_CTX_new", {-1}},
    {"EVP_KDF_CTX_set_params", {0}},
    {"EVP_KDF_derive", {0}},
    {"EVP_KDF_CTX_free", {0}}

};

// delta[Token][State] = next State
// Token: EVP_KDF_CTX_NEW = 0,
// EVP_KDF_CTX_SET_PARAMS = 1,
// DERIVE = 2,
// EVP_KDF_CTX_FREE = 3,
// STAR = 4
//
// States: UNINIT = 5, CTX_ATTACHED =1, PARAM_INIT = 2,
// DERIVED = 3, ERROR = 4, BOT = 0
const OpenSSLEVPKDFCTXDescription::OpenSSLEVPKDFState
    OpenSSLEVPKDFCTXDescription::delta[5][6] = {

        /* EVP_KDF_CTX_NEW */
        {OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::CTX_ATTACHED, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::CTX_ATTACHED},
        /* EVP_KDF_CTX_SET_PARAMS */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::PARAM_INIT,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR},
        /* DERIVE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR,
         OpenSSLEVPKDFState::DERIVED, OpenSSLEVPKDFState::DERIVED,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR},
        /* EVP_KDF_CTX_FREE */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::UNINIT,
         OpenSSLEVPKDFState::UNINIT, OpenSSLEVPKDFState::UNINIT,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR},

        /* STAR */
        {OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::CTX_ATTACHED,
         OpenSSLEVPKDFState::PARAM_INIT, OpenSSLEVPKDFState::DERIVED,
         OpenSSLEVPKDFState::ERROR, OpenSSLEVPKDFState::ERROR},
};

bool OpenSSLEVPKDFCTXDescription::isFactoryFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) != OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFCTXDescription::isConsumingFunction(
    const std::string &F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) == OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFCTXDescription::isAPIFunction(const std::string &F) const {
  return OpenSSLEVPKDFFuncs.find(F) != OpenSSLEVPKDFFuncs.end();
}

TypeStateDescription::State
OpenSSLEVPKDFCTXDescription::getNextState(std::string Tok,
                                          TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto nameToTok = funcNameToToken(Tok);
    auto ret = delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        nameToTok)][S];

    // std::cout << "delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return ret;
  } else {
    return OpenSSLEVPKDFState::BOT;
  }
}
TypeStateDescription::State
OpenSSLEVPKDFCTXDescription::getNextState(const std::string &Tok,
                                          TypeStateDescription::State S,
                                          llvm::ImmutableCallSite CS) const {
  if (isAPIFunction(Tok)) {
    auto nameToTok = funcNameToToken(Tok);
    auto ret = delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        nameToTok)][S];

    if (nameToTok == OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW) {
      // require the kdf here to be in KDF_FETCHED state

      // requiredKDFState[make_pair(CS.getInstruction(), CS.getArgOperand(0))] =
      //     (OpenSSLEVPKDFDescription::OpenSSLEVPKDFState::KDF_FETCHED);
      // cout << "## Factory-Call: ";
      // cout.flush();
      // cout << llvmIRToShortString(CS.getInstruction()) << endl;
      auto kdfState =
          kdfAnalysisResults.resultAt(CS.getInstruction(), CS.getArgOperand(0));
      if (kdfState != OpenSSLEVPKDFDescription::OpenSSLEVPKDFState::KDF_FETCHED)
        return error();
    }
    // std::cout << "delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return ret;
  } else {
    return OpenSSLEVPKDFState::BOT;
  }
}

std::string OpenSSLEVPKDFCTXDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdf_ctx_st";
}

set<int>
OpenSSLEVPKDFCTXDescription::getConsumerParamIdx(const std::string &F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F);
  }
  return {};
}

set<int>
OpenSSLEVPKDFCTXDescription::getFactoryParamIdx(const std::string &F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

std::string OpenSSLEVPKDFCTXDescription::stateToString(
    TypeStateDescription::State S) const {
  switch (S) {
  case OpenSSLEVPKDFState::TOP:
    return "TOP";
    break;
  case OpenSSLEVPKDFState::UNINIT:
    return "UNINIT";
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
    llvm::report_fatal_error("received unknown state!");
    break;
  }
}

TypeStateDescription::State OpenSSLEVPKDFCTXDescription::bottom() const {
  return OpenSSLEVPKDFState::BOT;
}

TypeStateDescription::State OpenSSLEVPKDFCTXDescription::top() const {
  return OpenSSLEVPKDFState::TOP;
}

TypeStateDescription::State OpenSSLEVPKDFCTXDescription::uninit() const {
  return OpenSSLEVPKDFState::UNINIT;
}

TypeStateDescription::State OpenSSLEVPKDFCTXDescription::start() const {
  return OpenSSLEVPKDFState::CTX_ATTACHED;
}

TypeStateDescription::State OpenSSLEVPKDFCTXDescription::error() const {
  return OpenSSLEVPKDFState::ERROR;
}

OpenSSLEVPKDFCTXDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFCTXDescription::funcNameToToken(const std::string &F) const {
  if (F == "EVP_KDF_CTX_new")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW;
  else if (F == "EVP_KDF_CTX_set_params")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_SET_PARAMS;
  else if (F == "EVP_KDF_derive")
    return OpenSSLEVTKDFToken::DERIVE;

  else if (F == "EVP_KDF_CTX_free")
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_FREE;
  else
    return OpenSSLEVTKDFToken::STAR;
}

// bool OpenSSLEVPKDFCTXDescription::validateKDFConstraints(
//     IDESolver<const llvm::Instruction *, const llvm::Value *,
//               const llvm::Function *, const llvm::StructType *,
//               const llvm::Value *, int, LLVMBasedICFG> &kdfAnalysisResults,
//     std::function<void(const llvm::Instruction *, const llvm::Value *, int,
//                        int)>
//         onError) const {
//   for (auto &[stmtVal, expectedState] : requiredKDFState) {
//     auto actualState = kdfAnalysisResults.resultAt(stmtVal.first,
//     stmtVal.second); if(actualState != expectedState){
//       onError(stmtVal.first, stmtVal.second, expectedState, actualState);
//     }
//   }
// }
} // namespace psr
