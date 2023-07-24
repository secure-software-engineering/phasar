/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/BinaryEdgeFunction.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr::glca {

auto BinaryEdgeFunction::computeTarget(ByConstRef<l_t> Source) const -> l_t {
  if (LeftConst) {
    return performBinOp(Op, Const, Source, IDEGeneralizedLCADomain::MaxSetSize);
  }
  return performBinOp(Op, Source, Const, IDEGeneralizedLCADomain::MaxSetSize);
}

auto BinaryEdgeFunction::compose(EdgeFunctionRef<BinaryEdgeFunction> This,
                                 const EdgeFunction<l_t> &SecondFunction)
    -> EdgeFunction<l_t> {
  if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
    return Default;
  }
  return LCAEdgeFunctionComposer{This, SecondFunction};
}

auto BinaryEdgeFunction::join(EdgeFunctionRef<BinaryEdgeFunction> This,
                              const EdgeFunction<l_t> &OtherFunction)
    -> EdgeFunction<l_t> {
  if (auto Default =
          defaultJoinOrNull<l_t, IDEGeneralizedLCADomain::JoinThreshold>(
              This, OtherFunction)) {
    return Default;
  }

  /// XXX: Shouldn't this be JoinEdgeFunction<l_t,
  /// IDEGeneralizedLCADomain::JoinThreshold>::create(This, OtherFunction); ???
  return AllBottom<l_t>{};
}

bool BinaryEdgeFunction::operator==(
    const BinaryEdgeFunction &Other) const noexcept {
  return Other.Op == Op && Other.Const == Const && Other.LeftConst == LeftConst;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const BinaryEdgeFunction &EF) {
  return OS << "Binary_" << EF.Op;
}

} // namespace psr::glca
