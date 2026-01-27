/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_INITIALSEEDS_H
#define PHASAR_DATAFLOW_IFDSIDE_INITIALSEEDS_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/Support/Compiler.h"

#include <map>
#include <set>
#include <type_traits>

namespace psr {

/// \brief Represent the starting points of the analysis.
///
/// The initial facts that should hold at the entry points.
template <typename N, typename D, typename L> class InitialSeeds {
public:
  using GeneralizedSeeds = std::map<N, std::map<D, L>>;

  using n_t = N;
  using d_t = D;
  using l_t = L;

  InitialSeeds() = default;

  InitialSeeds(const std::map<N, std::set<D>> &Seeds)
    requires std::is_same_v<l_t, BinaryDomain>
  {
    for (const auto &[Node, Facts] : Seeds) {
      for (const auto &Fact : Facts) {
        this->Seeds[Node][Fact] = BinaryDomain::BOTTOM;
      }
    }
  }

  InitialSeeds(GeneralizedSeeds Seeds) : Seeds(std::move(Seeds)) {}

  void addSeed(N Node, D Fact)
    requires std::is_same_v<l_t, BinaryDomain>
  {
    addSeed(Node, Fact, BinaryDomain::BOTTOM);
  }

  void addSeed(N Node, D Fact, L Value) {
    Seeds[std::move(Node)].insert_or_assign(std::move(Fact), std::move(Value));
  }

  [[nodiscard]] size_t countInitialSeeds() const {
    size_t NumSeeds = 0;
    for (const auto &[Node, Facts] : Seeds) {
      NumSeeds += Facts.size();
    }
    return NumSeeds;
  }

  [[nodiscard]] size_t countInitialSeeds(N Node) const {
    auto Search = Seeds.find(Node);
    if (Search != Seeds.end()) {
      return Search->second.size();
    }
    return 0;
  }

  [[nodiscard]] bool containsInitialSeedsFor(N Node) const {
    return Seeds.count(Node);
  }

  [[nodiscard]] bool empty() const { return Seeds.empty(); }

  [[nodiscard]] const GeneralizedSeeds &getSeeds() const & { return Seeds; }
  [[nodiscard]] GeneralizedSeeds getSeeds() && { return std::move(Seeds); }

  void dump(llvm::raw_ostream &OS = llvm::errs()) const {
    OS << "======================== Initial Seeds ========================\n";
    for (const auto &[Node, Facts] : Seeds) {
      OS << "At " << NToString(Node) << '\n';
      for (const auto &[Fact, Value] : Facts) {
        OS << "> " << DToString(Fact) << " --> \\." << LToString(Value) << '\n';
      }
      OS << '\n';
    }
    OS << "========================== End Seeds ==========================\n";
  }

private:
  GeneralizedSeeds Seeds;
};

} // namespace psr

#endif
