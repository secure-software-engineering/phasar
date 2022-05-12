/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/TypecastEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr {

IDEGeneralizedLCA::l_t
TypecastEdgeFunction::computeTarget(IDEGeneralizedLCA::l_t Source) {
  return performTypecast(Source, Dest, Bits);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
TypecastEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  if (dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(SecondFunction.get())) {
    return shared_from_this();
  }
  return std::make_shared<LCAEdgeFunctionComposer>(shared_from_this(),
                                                   SecondFunction, MaxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
TypecastEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction) {
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), OtherFunction,
                                            MaxSize);
}

bool TypecastEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) const {
  if (this == Other.get()) {
    return true;
  }
  if (const auto *OtherTC = dynamic_cast<TypecastEdgeFunction *>(Other.get())) {
    return Bits == OtherTC->Bits && Dest == OtherTC->Dest;
  }
  return false;
}

void TypecastEdgeFunction::print(llvm::raw_ostream &OS,
                                 bool /*IsForDebug*/) const {
  OS << "TypecastEdgeFn[to=" << EdgeValue::typeToString(Dest)
     << "; bits=" << Bits << "]";
}

} // namespace psr
