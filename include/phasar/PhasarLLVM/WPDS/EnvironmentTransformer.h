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
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>

namespace psr {

template <typename V> class EnvTrafoToSemElem : public wali::SemElem {
private:
  std::shared_ptr<EdgeFunction<V>> F;
  const JoinLattice<V> &L;

public:
  EnvTrafoToSemElem(std::shared_ptr<EdgeFunction<V>> F, const JoinLattice<V> &L)
      : wali::SemElem(), F(F), L(L) {}
  virtual ~EnvTrafoToSemElem() = default;

  virtual std::ostream &print(std::ostream &os) const override {
    return os << *F;
  }

  virtual wali::sem_elem_t one() const override { return 0; }

  virtual wali::sem_elem_t zero() const override { return 0; }

  virtual wali::sem_elem_t extend(SemElem *se) override { return 0; }

  virtual wali::sem_elem_t combine(SemElem *se) override { return 0; }

  virtual bool equal(SemElem *se) const override { return false; }
};

} // namespace psr

#endif
