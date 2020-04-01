/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_JOINLATTICETOSEMIRINGELEM_H_
#define PHASAR_PHASARLLVM_WPDS_JOINLATTICETOSEMIRINGELEM_H_

#include <iosfwd>
#include <memory>

#include "wali/SemElem.hpp"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"

namespace psr {
/**
 * An idempotent semi-ring is one whose addition is idempotent
 *
 *    a + a = a
 *
 * that is (R, +, 0) - is a join-semilattice with zero.
 */
template <typename V> class JoinLatticeToSemiRingElem : public wali::SemElem {
public:
  std::shared_ptr<EdgeFunction<V>> F;
  JoinLattice<V> &L;
  V v;

  JoinLatticeToSemiRingElem(std::shared_ptr<EdgeFunction<V>> F,
                            JoinLattice<V> &L)
      : wali::SemElem(), F(F), L(L) {}
  virtual ~JoinLatticeToSemiRingElem() = default;

  std::ostream &print(std::ostream &os) const override { return os << *F; }

  wali::sem_elem_t one() const override {
    // std::cout << "JoinLatticeToSemiRingElem::one()" << std::endl;
    return wali::ref_ptr<JoinLatticeToSemiRingElem<V>>(
        new JoinLatticeToSemiRingElem(
            std::make_shared<AllBottom<V>>(L.bottomElement()), L));
  }

  wali::sem_elem_t zero() const override {
    // std::cout << "JoinLatticeToSemiRingElem::zero()" << std::endl;
    return wali::ref_ptr<JoinLatticeToSemiRingElem<V>>(
        new JoinLatticeToSemiRingElem(
            std::make_shared<AllTop<V>>(L.topElement()), L));
  }

  wali::sem_elem_t extend(SemElem *se) override {
    // std::cout << "JoinLatticeToSemiRingElem::extend()" << std::endl;
    auto ThisF = static_cast<JoinLatticeToSemiRingElem *>(this);
    auto ThatF = static_cast<JoinLatticeToSemiRingElem *>(se);
    return wali::ref_ptr<JoinLatticeToSemiRingElem<V>>(
        new JoinLatticeToSemiRingElem(ThisF->F->composeWith(ThatF->F), L));
  }

  wali::sem_elem_t combine(SemElem *se) override {
    // std::cout << "JoinLatticeToSemiRingElem::combine()" << std::endl;
    auto ThisF = static_cast<JoinLatticeToSemiRingElem *>(this);
    auto ThatF = static_cast<JoinLatticeToSemiRingElem *>(se);
    return wali::ref_ptr<JoinLatticeToSemiRingElem<V>>(
        new JoinLatticeToSemiRingElem(ThisF->F->joinWith(ThatF->F), L));
  }

  bool equal(SemElem *se) const override {
    // std::cout << "JoinLatticeToSemiRingElem::equal()" << std::endl;
    auto ThisF = static_cast<const JoinLatticeToSemiRingElem *>(this);
    auto ThatF = static_cast<const JoinLatticeToSemiRingElem *>(se);
    return ThisF->F->equal_to(ThatF->F);
  }
};

template <typename V>
std::ostream &operator<<(std::ostream &os,
                         const JoinLatticeToSemiRingElem<V> &ETS) {
  ETS.print(os);
  return os;
}

} // namespace psr

#endif
