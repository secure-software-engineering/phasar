/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/TypecastEdgeFunction.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCADomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr::glca {

IDEGeneralizedLCA::l_t
TypecastEdgeFunction::computeTarget(ByConstRef<l_t> Source) const {
  return performTypecast(Source, Dest, Bits);
}

auto TypecastEdgeFunction::compose(EdgeFunctionRef<TypecastEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction)
    -> EdgeFunction<l_t> {
  if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
    return Default;
  }
  return LCAEdgeFunctionComposer{This, SecondFunction};
}

auto TypecastEdgeFunction::join(EdgeFunctionRef<TypecastEdgeFunction> This,
                                const EdgeFunction<l_t> &OtherFunction)
    -> EdgeFunction<l_t> {
  if (auto Default =
          defaultJoinOrNull<l_t, IDEGeneralizedLCADomain::JoinThreshold>(
              This, OtherFunction)) {
    return Default;
  }

  return JoinEdgeFunction<l_t, IDEGeneralizedLCADomain::JoinThreshold>::create(
      This, OtherFunction);
}

} // namespace psr::glca

bool psr::glca::operator==(ByConstRef<TypecastEdgeFunction> LHS,
                           ByConstRef<TypecastEdgeFunction> RHS) noexcept {
  return LHS.Bits == RHS.Bits && LHS.Dest == RHS.Dest;
}

llvm::raw_ostream &psr::glca::operator<<(llvm::raw_ostream &OS,
                                         ByConstRef<TypecastEdgeFunction> EF) {
  return OS << "TypecastEdgeFn[to=" << EdgeValue::typeToString(EF.Dest)
            << "; bits=" << EF.Bits << "]";
}
