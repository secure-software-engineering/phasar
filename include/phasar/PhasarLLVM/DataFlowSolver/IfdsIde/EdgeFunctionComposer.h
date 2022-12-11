/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONCOMPOSER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONCOMPOSER_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"

#include <memory>

namespace psr {

/**
 * This abstract class models edge function composition. It holds two edge
 * functions. The edge function computation order is implemented as
 *  F -> G -> otherFunction
 * i.e. F is computed before G, G is computed before otherFunction.
 *
 * Note that an own implementation for the join function is required, since
 * this varies between different analyses, and is not implemented by this
 * class.
 * It is also advised to provide a more precise compose function, which is able
 * to reduce the result of the composition, rather than using the default
 * implementation. By default, an explicit composition is used. Such a function
 * definition can grow unduly large.
 */
template <typename L>
class EdgeFunctionComposer
    : public EdgeFunction<L>,
      public std::enable_shared_from_this<EdgeFunctionComposer<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  // For debug purpose only
  const unsigned EFComposerId;
  static inline unsigned CurrEFComposerId = 0; // NOLINT

protected:
  /// First edge function
  EdgeFunctionPtrType F;
  /// Second edge function
  EdgeFunctionPtrType G;

public:
  EdgeFunctionComposer(EdgeFunctionPtrType &F, EdgeFunctionPtrType &G)
      : EFComposerId(++CurrEFComposerId), F(F), G(G) {}

  ~EdgeFunctionComposer() override = default;

  /**
   * Target value computation is implemented as
   *     G(F(source))
   */
  L computeTarget(L Source) override {
    return G->computeTarget(F->computeTarget(Source));
  }

  /**
   * Function composition is implemented as an explicit composition, i.e.
   *     (secondFunction * G) * F = EFC(F, EFC(G , otherFunction))
   *
   * However, it is advised to immediately reduce the resulting edge function
   * by providing an own implementation of this function.
   */
  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    if (auto *EI = dynamic_cast<EdgeIdentity<L> *>(SecondFunction.get())) {
      return this->shared_from_this();
    }
    if (auto *AB = dynamic_cast<AllBottom<L> *>(SecondFunction.get())) {
      return this->shared_from_this();
    }
    return F->composeWith(G->composeWith(SecondFunction));
  }

  // virtual EdgeFunctionPtrType
  // joinWith(EdgeFunctionPtrType otherFunction) = 0;

  bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto EFC = dynamic_cast<EdgeFunctionComposer<L> *>(Other.get())) {
      return F->equal_to(EFC->F) && G->equal_to(EFC->G);
    }
    return false;
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug = false*/) const override {
    OS << "COMP[ " << F.get()->str() << " , " << G.get()->str()
       << " ] (EF:" << EFComposerId << ')';
  }
};

} // namespace psr

#endif
