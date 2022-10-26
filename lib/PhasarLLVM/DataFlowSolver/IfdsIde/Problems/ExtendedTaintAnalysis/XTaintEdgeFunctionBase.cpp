/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinConstEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"

namespace psr::XTaint {
EdgeFunctionBase::EdgeFunctionBase(EFKind Kind, BasicBlockOrdering &BBO)
    : BBO(BBO), Kind(Kind) {}

EdgeFunctionBase::EdgeFunctionPtrType
EdgeFunctionBase::composeWith(EdgeFunctionPtrType SecondFunction) {
  if (isEdgeIdentity(&*SecondFunction)) {
    return shared_from_this();
  }
  if (dynamic_cast<AllBottom<l_t> *>(&*SecondFunction)) {
    return SecondFunction;
  }
  if (dynamic_cast<AllTop<l_t> *>(&*SecondFunction)) {
    return shared_from_this();
  }
  if (dynamic_cast<GenEdgeFunction *>(&*SecondFunction)) {
    return SecondFunction;
  }
  if (&*SecondFunction == &*getAllSanitized()) {
    return SecondFunction;
  }

  return makeEF<ComposeEdgeFunction>(BBO, shared_from_this(), SecondFunction);
}
EdgeFunctionBase::EdgeFunctionPtrType
EdgeFunctionBase::joinWith(EdgeFunctionPtrType OtherFunction) {
  if (dynamic_cast<psr::AllBottom<l_t> *>(&*OtherFunction)) {
    return OtherFunction;
  }
  if (dynamic_cast<psr::AllTop<l_t> *>(&*OtherFunction)) {
    return shared_from_this();
  }
  if (&*OtherFunction == &*getAllSanitized()) {
    return shared_from_this();
  }

  // if (isEdgeIdentity(&*OtherFunction)) {
  //   return getAllBot();
  // }

  if (auto *Gen = dynamic_cast<GenEdgeFunction *>(&*OtherFunction)) {
    if (Gen->getSanitizer() == nullptr) {
      return OtherFunction;
    }
    return makeEF<JoinConstEdgeFunction>(BBO, shared_from_this(),
                                         Gen->getSanitizer());
  }

  if (this == &*OtherFunction || equal_to(OtherFunction)) {
    return shared_from_this();
  }

  return JoinEdgeFunction::create(BBO, shared_from_this(), OtherFunction);
}
} // namespace psr::XTaint
