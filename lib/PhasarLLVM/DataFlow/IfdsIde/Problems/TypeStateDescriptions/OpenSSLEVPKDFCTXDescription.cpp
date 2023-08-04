/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"

#include <set>
#include <string>

namespace psr {

// Return value is modeled as -1
static const std::map<llvm::StringRef, std::set<int>> OpenSSLEVPKDFFuncs = {
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
const OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::Delta[5][6] = {

    /* EVP_KDF_CTX_NEW */
    {OpenSSLEVPKDFCTXState::CTX_ATTACHED, OpenSSLEVPKDFCTXState::CTX_ATTACHED,
     OpenSSLEVPKDFCTXState::CTX_ATTACHED, OpenSSLEVPKDFCTXState::CTX_ATTACHED,
     OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::CTX_ATTACHED},
    /* EVP_KDF_CTX_SET_PARAMS */
    {OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::PARAM_INIT,
     OpenSSLEVPKDFCTXState::PARAM_INIT, OpenSSLEVPKDFCTXState::PARAM_INIT,
     OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::ERROR},
    /* DERIVE */
    {OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::ERROR,
     OpenSSLEVPKDFCTXState::DERIVED, OpenSSLEVPKDFCTXState::DERIVED,
     OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::ERROR},
    /* EVP_KDF_CTX_FREE */
    {OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::UNINIT,
     OpenSSLEVPKDFCTXState::UNINIT, OpenSSLEVPKDFCTXState::UNINIT,
     OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::ERROR},

    /* STAR */
    {OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::CTX_ATTACHED,
     OpenSSLEVPKDFCTXState::PARAM_INIT, OpenSSLEVPKDFCTXState::DERIVED,
     OpenSSLEVPKDFCTXState::ERROR, OpenSSLEVPKDFCTXState::ERROR},
};

bool OpenSSLEVPKDFCTXDescription::isFactoryFunction(llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) != OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFCTXDescription::isConsumingFunction(llvm::StringRef F) const {
  if (isAPIFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F).find(-1) == OpenSSLEVPKDFFuncs.at(F).end();
  }
  return false;
}

bool OpenSSLEVPKDFCTXDescription::isAPIFunction(llvm::StringRef F) const {
  return OpenSSLEVPKDFFuncs.find(F) != OpenSSLEVPKDFFuncs.end();
}

OpenSSLEVPKDFCTXState
OpenSSLEVPKDFCTXDescription::getNextState(llvm::StringRef Tok,
                                          TypeStateDescription::State S) const {
  if (isAPIFunction(Tok)) {
    auto NameToTok = funcNameToToken(Tok);
    auto Ret = Delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        NameToTok)][int(S)];

    // std::cout << "delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return Ret;
  }
  return OpenSSLEVPKDFCTXState::BOT;
}
OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::getNextState(
    llvm::StringRef Tok, TypeStateDescription::State S,
    const llvm::CallBase *CallSite) const {
  if (isAPIFunction(Tok)) {
    auto NameToTok = funcNameToToken(Tok);
    auto Ret = Delta[static_cast<std::underlying_type_t<OpenSSLEVTKDFToken>>(
        NameToTok)][int(S)];

    if (NameToTok == OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW) {
      // require the kdf here to be in KDF_FETCHED state

      // requiredKDFState[make_pair(CS.getInstruction(), CS.getArgOperand(0))] =
      //     (OpenSSLEVPKDFDescription::OpenSSLEVPKDFCTXState::KDF_FETCHED);
      // cout << "## Factory-Call: ";
      // cout.flush();
      // cout << llvmIRToShortString(CS.getInstruction()) << endl;
      auto KdfState =
          KDFAnalysisResults.resultAt(CallSite, CallSite->getArgOperand(0));
      if (KdfState != OpenSSLEVPKDFState::KDF_FETCHED) {
        return error();
      }
    }
    // std::cout << "delta[" << Tok << ", " << stateToString(S)
    //           << "] = " << stateToString(ret) << std::endl;
    return Ret;
  }
  return OpenSSLEVPKDFCTXState::BOT;
}

std::string OpenSSLEVPKDFCTXDescription::getTypeNameOfInterest() const {
  return "struct.evp_kdf_ctx_st";
}

std::set<int>
OpenSSLEVPKDFCTXDescription::getConsumerParamIdx(llvm::StringRef F) const {
  if (isConsumingFunction(F)) {
    return OpenSSLEVPKDFFuncs.at(F);
  }
  return {};
}

std::set<int>
OpenSSLEVPKDFCTXDescription::getFactoryParamIdx(llvm::StringRef F) const {
  if (isFactoryFunction(F)) {
    // Trivial here, since we only generate via return value
    return {-1};
  }
  return {};
}

llvm::StringRef to_string(OpenSSLEVPKDFCTXState State) noexcept {
  switch (State) {
  case OpenSSLEVPKDFCTXState::TOP:
    return "TOP";
    break;
  case OpenSSLEVPKDFCTXState::UNINIT:
    return "UNINIT";
    break;

  case OpenSSLEVPKDFCTXState::CTX_ATTACHED:
    return "CTX_ATTACHED";
    break;
  case OpenSSLEVPKDFCTXState::PARAM_INIT:
    return "PARAM_INIT";
    break;
  case OpenSSLEVPKDFCTXState::DERIVED:
    return "DERIVED";
    break;
  case OpenSSLEVPKDFCTXState::ERROR:
    return "ERROR";
    break;
  case OpenSSLEVPKDFCTXState::BOT:
    return "BOT";
    break;
  }

  llvm::report_fatal_error("received unknown state!");
}

OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::bottom() const {
  return OpenSSLEVPKDFCTXState::BOT;
}

OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::top() const {
  return OpenSSLEVPKDFCTXState::TOP;
}

OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::uninit() const {
  return OpenSSLEVPKDFCTXState::UNINIT;
}

OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::start() const {
  return OpenSSLEVPKDFCTXState::CTX_ATTACHED;
}

OpenSSLEVPKDFCTXState OpenSSLEVPKDFCTXDescription::error() const {
  return OpenSSLEVPKDFCTXState::ERROR;
}

OpenSSLEVPKDFCTXDescription::OpenSSLEVTKDFToken
OpenSSLEVPKDFCTXDescription::funcNameToToken(llvm::StringRef F) {
  if (F == "EVP_KDF_CTX_new") {
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_NEW;
  }
  if (F == "EVP_KDF_CTX_set_params") {
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_SET_PARAMS;
  }
  if (F == "EVP_KDF_derive") {
    return OpenSSLEVTKDFToken::DERIVE;
  }
  if (F == "EVP_KDF_CTX_free") {
    return OpenSSLEVTKDFToken::EVP_KDF_CTX_FREE;
  }
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
