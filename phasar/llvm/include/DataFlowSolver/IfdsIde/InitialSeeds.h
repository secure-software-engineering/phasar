/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_INITIALSEEDS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_INITIALSEEDS_H

#include <map>
#include <set>
#include <type_traits>

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename N, typename D, typename L> class InitialSeeds {
public:
  using GeneralizedSeeds = std::map<N, std::map<D, L>>;

  InitialSeeds() = default;

  template <typename LL = L,
            typename = std::enable_if_t<std::is_same_v<LL, BinaryDomain>>>
  InitialSeeds(const std::map<N, std::set<D>> &Seeds) {
    for (const auto &[Node, Facts] : Seeds) {
      for (const auto &Fact : Facts) {
        this->Seeds[Node][Fact] = BinaryDomain::TOP;
      }
    }
  }

  InitialSeeds(GeneralizedSeeds Seeds) : Seeds(std::move(Seeds)) {}

  template <typename LL = L,
            typename = std::enable_if_t<std::is_same_v<LL, BinaryDomain>>>
  void addSeed(N Node, D Fact) {
    addSeed(Node, Fact, BinaryDomain::TOP);
  }

  void addSeed(N Node, D Fact, L Value) {
    Seeds[Node][Fact] = std::move(Value);
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

  void dump(llvm::raw_ostream &OS = llvm::errs()) {

    auto printNode = [&](auto &&Node) { // NOLINT
      if constexpr (std::is_same_v<const llvm::Instruction *, N>) {
        OS << llvmIRToString(Node);
      } else {
        OS << Node;
      }
    };

    auto printFact = [&](auto &&Node) { // NOLINT
      if constexpr (std::is_same_v<const llvm::Value *, D>) {
        OS << llvmIRToString(Node);
      } else {
        OS << Node;
      }
    };

    OS << "======================== Initial Seeds ========================\n";
    for (const auto &[Node, Facts] : Seeds) {
      OS << "At ";
      printNode(Node);
      OS << "\n";
      for (const auto &[Fact, Value] : Facts) {
        OS << "> ";
        printFact(Fact);
        OS << " --> \\." << Value << "\n";
      }
      OS << "\n";
    }
    OS << "========================== End Seeds ==========================\n";
  }

private:
  GeneralizedSeeds Seeds;
};

} // namespace psr

#endif
