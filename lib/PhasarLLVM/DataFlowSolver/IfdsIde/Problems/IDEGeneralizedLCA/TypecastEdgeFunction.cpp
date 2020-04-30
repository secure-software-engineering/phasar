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
TypecastEdgeFunction::computeTarget(IDEGeneralizedLCA::l_t source) {
  return performTypecast(source, dest, bits);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
TypecastEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> secondFunction) {
  if (dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(secondFunction.get()))
    return shared_from_this();
  return std::make_shared<LCAEdgeFunctionComposer>(shared_from_this(),
                                                   secondFunction, maxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
TypecastEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> otherFunction) {
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

bool TypecastEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> other) const {
  if (this == other.get())
    return true;
  if (auto otherTC = dynamic_cast<TypecastEdgeFunction *>(other.get())) {
    return bits == otherTC->bits && dest == otherTC->dest;
  }
  return false;
}

void TypecastEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "TypecastEdgeFn[to=" << EdgeValue::typeToString(dest)
     << "; bits=" << bits << "]";
}

} // namespace psr
