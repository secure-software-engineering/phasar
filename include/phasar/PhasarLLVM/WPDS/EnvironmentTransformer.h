/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_ENVIRONMENTTRANSFORMER_H_
#define PHASAR_PHASARLLVM_WPDS_ENVIRONMENTTRANSFORMER_H_

#include <iosfwd>
#include <memory>

#include <wali/SemElem.hpp>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>

namespace psr {
/**
 * An idempotent semi-ring is one whose addition is idempotent
 *
 *    a + a = a
 *
 * that is (R, +, 0) - is a join-semilattice with zero.
 */
template <typename V> class EnvTrafoToSemElem : public wali::SemElem {
public:
  std::shared_ptr<EdgeFunction<V>> F;
  JoinLattice<V> &L;
  V v;

  EnvTrafoToSemElem(std::shared_ptr<EdgeFunction<V>> F, JoinLattice<V> &L)
      : wali::SemElem(), F(F), L(L) {}
  virtual ~EnvTrafoToSemElem() = default;

  std::ostream &print(std::ostream &os) const override { return os << *F; }

  wali::sem_elem_t one() const override {
    std::cout << "EnvTrafoToSemElem::one()" << std::endl;
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(new EnvTrafoToSemElem(
        std::make_shared<AllBottom<V>>(L.bottomElement()), L));
  }

  wali::sem_elem_t zero() const override {
    std::cout << "EnvTrafoToSemElem::zero()" << std::endl;
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(
        // new EnvTrafoToSemElem(EdgeIdentity<V>::getInstance(), L));
        new EnvTrafoToSemElem(std::make_shared<AllTop<V>>(L.topElement()), L));
  }

  wali::sem_elem_t extend(SemElem *se) override {
    std::cout << "EnvTrafoToSemElem::extend()" << std::endl;
    auto ThisF = dynamic_cast<EnvTrafoToSemElem *>(this);
    auto ThatF = dynamic_cast<EnvTrafoToSemElem *>(se);
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(
        new EnvTrafoToSemElem(ThisF->F->composeWith(ThatF->F), L));
  }

  wali::sem_elem_t combine(SemElem *se) override {
    std::cout << "EnvTrafoToSemElem::combine()" << std::endl;
    auto ThisF = dynamic_cast<EnvTrafoToSemElem *>(this);
    auto ThatF = dynamic_cast<EnvTrafoToSemElem *>(se);
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(
        new EnvTrafoToSemElem(ThisF->F->joinWith(ThatF->F), L));
  }

  bool equal(SemElem *se) const override {
    std::cout << "EnvTrafoToSemElem::equal()" << std::endl;
    auto ThisF = dynamic_cast<const EnvTrafoToSemElem *>(this);
    auto ThatF = dynamic_cast<const EnvTrafoToSemElem *>(se);
    return ThisF->F->equal_to(ThatF->F);
  }
};

template <typename V>
std::ostream &operator<<(std::ostream &os, const EnvTrafoToSemElem<V> &ETS) {
  ETS.print(os);
  return os;
}

} // namespace psr

#endif
