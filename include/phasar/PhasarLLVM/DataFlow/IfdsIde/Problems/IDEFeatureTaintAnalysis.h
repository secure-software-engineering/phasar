/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEFEATURETAINTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEFEATURETAINTANALYSIS_H

#include "phasar/DataFlow/IfdsIde/DefaultEdgeFunctionSingletonCache.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace llvm {
class GlobalVariable;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

struct IDEFeatureTaintEdgeFact {
  llvm::SmallBitVector Taints;

  static llvm::SmallBitVector fromBits(uintptr_t Bits) {
#if __has_builtin(__builtin_constant_p)
    if (__builtin_constant_p(Bits) && Bits == 0) {
      return {};
    }
#endif

    llvm::SmallBitVector Ret(llvm::findLastSet(Bits) + 1);
    Ret.setBitsInMask((const uint32_t *)&Bits, sizeof(Bits));
    return Ret;
  }

  IDEFeatureTaintEdgeFact(llvm::SmallBitVector &&Taints) noexcept
      : Taints(std::move(Taints)) {}
  IDEFeatureTaintEdgeFact(const llvm::SmallBitVector &Taints)
      : Taints(Taints) {}
  IDEFeatureTaintEdgeFact(uintptr_t Taints) noexcept
      : Taints(fromBits(Taints)) {}
  explicit IDEFeatureTaintEdgeFact() noexcept { Taints.invalid(); }

  void unionWith(uintptr_t Facts) {
    auto RequiredSize = llvm::findLastSet(Facts) + 1;
    if (RequiredSize > Taints.size()) {
      Taints.resize(RequiredSize);
    }
    Taints.setBitsInMask((const uint32_t *)&Facts, sizeof(Facts));
  }
  void unionWith(const IDEFeatureTaintEdgeFact &Facts) {
    if (Facts.isTop()) {
      return;
    }
    Taints |= Facts.Taints;
  }

  /// Checks whether this set contains no facts.
  /// Note: Top also counts as empty
  [[nodiscard]] inline bool empty() const noexcept {
    return isTop() || Taints.none();
  }
  /// Returns the number of facts in this set.
  /// Note: Top counts as empty
  [[nodiscard]] inline size_t size() const noexcept {
    if (isTop()) {
      return 0;
    }
    return Taints.count();
  }

  [[nodiscard]] inline bool isBottom() const noexcept { return Taints.empty(); }
  [[nodiscard]] inline bool isTop() const noexcept {
    return Taints.isInvalid();
  }

  friend llvm::hash_code
  hash_value(const IDEFeatureTaintEdgeFact &BV) noexcept {
    if (BV.Taints.empty()) {
      return {};
    }
    uintptr_t Buf;
    auto Words = BV.Taints.getData(Buf);
    size_t Idx = Words.size();
    while (Idx && Words[Idx - 1] == 0) {
      --Idx;
    }
    return llvm::hash_combine_range(Words.begin(),
                                    std::next(Words.begin(), Idx));
  }

  friend bool operator==(const IDEFeatureTaintEdgeFact &Lhs,
                         const IDEFeatureTaintEdgeFact &Rhs) noexcept {
    bool LeftEmpty = Lhs.Taints.none();
    bool RightEmpty = Rhs.Taints.none();
    if (LeftEmpty || RightEmpty) {
      return LeftEmpty == RightEmpty;
    }
    // Check, whether Lhs and Rhs actually have the same bits set and not
    // whether their internal representation is exactly identitcal

    uintptr_t LBuf, RBuf;
    auto LhsWords = Lhs.Taints.getData(LBuf);
    auto RhsWords = Rhs.Taints.getData(RBuf);
    if (LhsWords.size() == RhsWords.size()) {
      return LhsWords == RhsWords;
    }
    auto MinSize = std::min(LhsWords.size(), RhsWords.size());
    if (LhsWords.slice(0, MinSize) != RhsWords.slice(0, MinSize)) {
      return false;
    }
    auto Rest = (LhsWords.size() > RhsWords.size() ? LhsWords : RhsWords)
                    .slice(MinSize);
    return std::all_of(Rest.begin(), Rest.end(),
                       [](auto Word) { return Word == 0; });
  }

  friend bool operator!=(const IDEFeatureTaintEdgeFact &Lhs,
                         const IDEFeatureTaintEdgeFact &Rhs) noexcept {
    return !(Lhs == Rhs);
  }

  template <typename E> [[nodiscard]] std::string str() const {
    auto BV = BitVectorSet<E, llvm::SmallBitVector>::fromBits(Taints);
    return LToString(BV);
  }

  template <typename E> [[nodiscard]] auto toBVSet() const {
    if (isTop()) {
      return BitVectorSet<E, llvm::SmallBitVector>();
    }
    return BitVectorSet<E, llvm::SmallBitVector>::fromBits(Taints);
  }

  template <typename E> [[nodiscard]] auto toSet() const {
    std::set<E> Ret;
    if (isTop()) {
      return Ret;
    }

    for (const auto &Elem : this->template toBVSet<E>()) {
      Ret.insert(Elem);
    }
    return Ret;
  }
};

std::string LToString(const IDEFeatureTaintEdgeFact &EdgeFact);

template <> struct JoinLatticeTraits<IDEFeatureTaintEdgeFact> {
  inline static IDEFeatureTaintEdgeFact top() {
    IDEFeatureTaintEdgeFact Ret{};
    return Ret;
  }
  inline static IDEFeatureTaintEdgeFact bottom() {
    // TODO
    return 0;
  }
  inline static IDEFeatureTaintEdgeFact join(const IDEFeatureTaintEdgeFact &L,
                                             const IDEFeatureTaintEdgeFact &R) {
    if (L.isTop()) {
      return R;
    }
    if (R.isTop()) {
      return L;
    }
    auto Ret = L;
    Ret.Taints |= R.Taints;
    return Ret;
  }
};

struct IDEFeatureTaintAnalysisDomain : LLVMAnalysisDomainDefault {
  using l_t = IDEFeatureTaintEdgeFact;
};

class FeatureTaintGenerator {
public:
  using InstOrGlobal =
      std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>;

  using GenerateTaintsFn =
      llvm::unique_function<IDEFeatureTaintEdgeFact(InstOrGlobal) const>;
  using IsSourceFn = llvm::unique_function<bool(InstOrGlobal) const>;
  using PrinterFn =
      llvm::unique_function<std::string(const IDEFeatureTaintEdgeFact &) const>;

  template <typename EdgeFactGenerator>
  static GenerateTaintsFn createGenerateTaints(EdgeFactGenerator &&EFGen) {
    return [EFGen{std::forward<EdgeFactGenerator>(EFGen)}](InstOrGlobal IG) {
      const auto &TaintSet = std::invoke(EFGen, IG);
      BitVectorSet<ElementType<decltype(TaintSet)>, llvm::SmallBitVector> BV(
          llvm::adl_begin(TaintSet), llvm::adl_end(TaintSet));

      auto Ret = IDEFeatureTaintEdgeFact{std::move(BV).getBits()};

      // llvm::errs() << "generateTaints: " << LToString(Ret) << '\n';
      return Ret;
    };
  }

  template <typename EdgeFactGenerator>
  static PrinterFn createEdgeFactPrinter() {
    using ElemTy =
        ElementType<std::invoke_result_t<EdgeFactGenerator, InstOrGlobal>>;
    return [](const IDEFeatureTaintEdgeFact &Fact) {
      auto BV =
          BitVectorSet<ElemTy, llvm::SmallBitVector>::fromBits(Fact.Taints);
      return LToString(BV);
    };
  }

  FeatureTaintGenerator(IsSourceFn IsFeatureSource,
                        GenerateTaintsFn GenerateTaints, PrinterFn Printer)
      : IsFeatureSource(std::move(IsFeatureSource)),
        GenerateTaints(std::move(GenerateTaints)), Printer(std::move(Printer)) {
  }

  template <typename EdgeFactGenerator,
            typename = std::enable_if_t<!std::is_same_v<
                FeatureTaintGenerator, std::decay_t<EdgeFactGenerator>>>>
  FeatureTaintGenerator(EdgeFactGenerator &&EFGen)
      : IsFeatureSource([EFGen{EFGen}](InstOrGlobal IG) {
          return !llvm::empty(std::invoke(EFGen, IG));
        }),
        GenerateTaints(
            createGenerateTaints(std::forward<EdgeFactGenerator>(EFGen))),
        Printer(createEdgeFactPrinter<EdgeFactGenerator>()) {}

  template <typename SourceDetector, typename EdgeFactGenerator>
  FeatureTaintGenerator(SourceDetector &&SrcDetector, EdgeFactGenerator &&EFGen)
      : IsFeatureSource(std::forward<SourceDetector>(SrcDetector)),
        GenerateTaints(
            createGenerateTaints(std::forward<EdgeFactGenerator>(EFGen))),
        Printer(createEdgeFactPrinter<EdgeFactGenerator>()) {}

  [[nodiscard]] bool isSource(InstOrGlobal IG) const {
    return IsFeatureSource(IG);
  }

  [[nodiscard]] IDEFeatureTaintEdgeFact
  getGeneratedTaintsAt(InstOrGlobal IG) const {
    return GenerateTaints(IG);
  }

  [[nodiscard]] std::string
  toString(const IDEFeatureTaintEdgeFact &Fact) const {
    return Printer(Fact);
  }

private:
  IsSourceFn IsFeatureSource;
  GenerateTaintsFn GenerateTaints;
  PrinterFn Printer;
};

class IDEFeatureTaintAnalysis
    : public IDETabulationProblem<IDEFeatureTaintAnalysisDomain> {

public:
  IDEFeatureTaintAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                          std::vector<std::string> EntryPoints,
                          FeatureTaintGenerator &&TaintGen);

  template <typename EdgeFactGenerator>
  IDEFeatureTaintAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                          std::vector<std::string> EntryPoints,
                          EdgeFactGenerator &&EFGen)
      : IDEFeatureTaintAnalysis(
            IRDB, PT, std::move(EntryPoints),
            FeatureTaintGenerator(std::forward<EdgeFactGenerator>(EFGen))) {}

  template <typename SourceDetector, typename EdgeFactGenerator>
  IDEFeatureTaintAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                          std::vector<std::string> EntryPoints,
                          SourceDetector &&SrcDetector,
                          EdgeFactGenerator &&EFGen)
      : IDEFeatureTaintAnalysis(
            IRDB, PT, std::move(EntryPoints),
            FeatureTaintGenerator(std::forward<SourceDetector>(SrcDetector),
                                  std::forward<EdgeFactGenerator>(EFGen))) {}

  IDEFeatureTaintAnalysis(const IDEFeatureTaintAnalysis &) = delete;
  IDEFeatureTaintAnalysis &operator=(const IDEFeatureTaintAnalysis &) = delete;

  IDEFeatureTaintAnalysis(IDEFeatureTaintAnalysis &&) noexcept = default;
  IDEFeatureTaintAnalysis &
  operator=(IDEFeatureTaintAnalysis &&) noexcept = delete;

  // The EF Caches are incomplete, so move the dtor into the .cpp
  ~IDEFeatureTaintAnalysis() override;

  //////////////////////////////////////////////////////////////////////////////
  ///                              Flow Functions
  //////////////////////////////////////////////////////////////////////////////

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) override;
  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override;
  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override;

  //////////////////////////////////////////////////////////////////////////////
  ///                              Edge Functions
  //////////////////////////////////////////////////////////////////////////////

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override;

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction,
                                        d_t DestNode) override;

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitStmt, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override;

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                           d_t SuccNode) override;

  //////////////////////////////////////////////////////////////////////////////
  ///                                  Misc
  //////////////////////////////////////////////////////////////////////////////

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  bool isZeroValue(d_t FlowFact) const noexcept override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS = llvm::outs()) override;

  EdgeFunction<l_t> extend(const EdgeFunction<l_t> &FirstEF,
                           const EdgeFunction<l_t> &SecondEF) override;
  EdgeFunction<l_t> combine(const EdgeFunction<l_t> &FirstEF,
                            const EdgeFunction<l_t> &OtherEF) override;

private:
  FeatureTaintGenerator TaintGen;
  LLVMAliasInfoRef PT;

  struct GenerateEF;
  struct AddFactsEF;
  DefaultEdgeFunctionSingletonCacheImpl<GenerateEF, l_t> GenEFCache;
  DefaultEdgeFunctionSingletonCacheImpl<AddFactsEF, l_t> AddEFCache;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEFEATURETAINTANALYSIS_H
