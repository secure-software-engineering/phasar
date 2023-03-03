/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/TypeTraits.h"

using Val = decltype(std::declval<psr::AllTop<const llvm::Value *>>() ==
                     std::declval<psr::AllTop<const llvm::Value *>>());

static_assert(psr::IsEqualityComparable<psr::AllTop<const llvm::Value *>>);

// #include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
// #include
// "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

// namespace psr::glca {

// std::shared_ptr<AllBot::type> AllBot::getInstance() {
//   static std::shared_ptr<type> Ret =
//       std::make_shared<type>(IDEGeneralizedLCA::l_t{nullptr});
//   return Ret;
// }

// bool AllBot::isBot(const EdgeFunction<IDEGeneralizedLCA::l_t> *EdgeFn,
//                    bool NonRec) {
//   if (EdgeFn == nullptr) {
//     return false;
//   }
//   if (EdgeFn == getInstance().get()) {
//     return true;
//   }
//   if (dynamic_cast<const type *>(EdgeFn)) {
//     return true;
//   }
//   if (!NonRec) {
//     if (const auto *JoinEFn = dynamic_cast<const JoinEdgeFunction *>(EdgeFn))
//     {
//       return isBot(JoinEFn->getFirst(), true) &&
//              isBot(JoinEFn->getSecond(), true);
//     }
//   }
//   return false;
// }

// bool AllBot::isBot(
//     const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &EdgeFn,
//     bool NonRec) {
//   return isBot(EdgeFn.get(), NonRec);
// }

//} // namespace psr::glca
