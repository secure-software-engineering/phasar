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

template <typename V>
class EnvTrafoToSemElem : public wali::SemElem {
 private:
  std::shared_ptr<EdgeFunction<V>> F;
  const JoinLattice<V> &L;

 public:
  EnvTrafoToSemElem(std::shared_ptr<EdgeFunction<V>> F, const JoinLattice<V> &L)
      : wali::SemElem(), F(F), L(L) {}
  virtual ~EnvTrafoToSemElem() = default;

  std::ostream &print(std::ostream &os) const override { return os << *F; }

  wali::sem_elem_t one() const override {
    std::cout << "EnvTrafoToSemElem::one()\n";
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(
        new EnvTrafoToSemElem(EdgeIdentity<V>::getInstance(), L));
  }

  wali::sem_elem_t zero() const override {
    std::cout << "EnvTrafoToSemElem::zero()\n";
    return wali::ref_ptr<EnvTrafoToSemElem<V>>(
        new EnvTrafoToSemElem(std::make_shared<AllBottom<V>>(false), L));
  }

  wali::sem_elem_t extend(SemElem *se) override {
    std::cout << "EnvTrafoToSemElem::extend()\n";
    return this;
  }

  wali::sem_elem_t combine(SemElem *se) override {
    std::cout << "EnvTrafoToSemElem::combine()\n";
    return this;
  }

  bool equal(SemElem *se) const override {
    std::cout << "EnvTrafoToSemElem::equal()\n";
    return this == se;
  }
};

template <typename V>
std::ostream &operator<< (std::ostream &os, const EnvTrafoToSemElem<V> &ETS) {
  ETS.print(os);
  return os;
}

}  // namespace psr

#endif
