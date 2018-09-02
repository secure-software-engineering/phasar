/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONCOMPOSER_H
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONCOMPOSER_H

#include <gtest/gtest_prod.h>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>

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
template <typename V>
class EdgeFunctionComposer
    : public EdgeFunction<V>,
      public std::enable_shared_from_this<EdgeFunctionComposer<V>> {
private:
  // For debug purpose only
  const unsigned EFComposer_Id;
  static unsigned CurrEFComposer_Id;

protected:
  /// First edge function
  std::shared_ptr<EdgeFunction<V>> F;
  /// Second edge function
  std::shared_ptr<EdgeFunction<V>> G;

public:
  EdgeFunctionComposer(std::shared_ptr<EdgeFunction<V>> F,
                       std::shared_ptr<EdgeFunction<V>> G)
      : EFComposer_Id(++CurrEFComposer_Id), F(F), G(G) {}

  /**
   * Target value computation is implemented as
   *     G(F(source))
   */
  virtual V computeTarget(V source) override {
    return G->computeTarget(F->computeTarget(source));
  }

  /**
   * Function composition is implemented as an explicit composition, i.e.
   *     (secondFunction * G) * F = EFC(F, EFC(G , otherFunction))
   *
   * However, it is advised to immediately reduce the resulting edge function
   * by providing an own implementation of this function.
   */
  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) override {
    if (auto *EI = dynamic_cast<EdgeIdentity<V> *>(secondFunction.get())) {
      return this->shared_from_this();
    }
    return F->composeWith(G->composeWith(secondFunction));
  }

  // virtual std::shared_ptr<EdgeFunction<V>>
  // joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) = 0;

  virtual bool equal_to(std::shared_ptr<EdgeFunction<V>> other) const override {
    if (auto EFC = dynamic_cast<EdgeFunctionComposer<V> *>(other.get())) {
      return F->equal_to(EFC->F) && G->equal_to(EFC->G);
    }
    return false;
  }

  virtual void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "EFComposer_" << EFComposer_Id << "[ " << F.get()->str() << " , "
       << G.get()->str() << " ]";
  }
};

template <typename V> unsigned EdgeFunctionComposer<V>::CurrEFComposer_Id = 0;

} // namespace psr

#endif
