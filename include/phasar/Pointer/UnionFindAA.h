#pragma once

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/Pointer/CallingContextConstructor.h"
#include "phasar/Pointer/PointerAssignmentGraph.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/PointerUtils.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedArray.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/UnionFind.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

#include <concepts>
#include <functional>
#include <type_traits>

namespace psr {

/// Base interface for results of a union-find alias-analysis.
template <typename T>
concept UnionFindAAResult = requires(const T &Result, ValueId Var) {
  { T::isCached() } noexcept -> std::convertible_to<bool>;
  { Result.getRawAliasSet(Var) } -> std::convertible_to<RawAliasSet<ValueId>>;
  { Result.mayAlias(Var, Var) } -> std::convertible_to<bool>;
  { Result.size() } noexcept -> std::convertible_to<size_t>;
};

/// The intersection of two (different) independent union-find alias-analysis
/// results.
/// Use this to achieve better precision than feasible with a single analysis.
template <UnionFindAAResult FirstT, UnionFindAAResult SecondT>
struct UnionFindAAResultIntersection {
  [[nodiscard]] static constexpr bool isCached() noexcept {
    // The set-intersection is not cached
    return false;
  }

  [[nodiscard]] constexpr size_t size() const noexcept {
    assert(
        First.size() == Second.size() &&
        "Only alias-results on the same ValueCompressor should be intersected");
    return First.size();
  }

  [[nodiscard]] RawAliasSet<ValueId> getRawAliasSet(ValueId Var) const {
    auto ResultSet = First.getRawAliasSet(Var);
    ResultSet &= Second.getRawAliasSet(Var);
    return ResultSet;
  }

  [[nodiscard]] constexpr bool mayAlias(ValueId Var1, ValueId Var2) const {
    return First.mayAlias(Var1, Var2) && Second.mayAlias(Var1, Var2);
  }

  [[no_unique_address]] FirstT First;
  [[no_unique_address]] SecondT Second;
};

template <pag::PBStrategy FirstT, pag::CanOnAddEdge SecondT>
  requires(std::same_as<typename FirstT::n_t, typename SecondT::n_t> &&
           std::same_as<typename FirstT::v_t, typename SecondT::v_t> &&
           std::same_as<typename FirstT::f_t, typename SecondT::f_t> &&
           std::same_as<typename FirstT::db_t, typename SecondT::db_t>)
struct UnionFindAACombinator
    : public pag::PBStrategyCombinator<FirstT, SecondT> {

  constexpr UnionFindAACombinator(std::convertible_to<FirstT> auto &&First,
                                  std::convertible_to<SecondT> auto &&Second)
      : pag::PBStrategyCombinator<FirstT, SecondT>{PSR_FWD(First),
                                                   PSR_FWD(Second)} {}

  [[nodiscard]] UnionFindAAResult auto consumeAAResults(size_t NumVars) && {
    return UnionFindAAResultIntersection{
        std::move(this->First).consumeAAResults(NumVars),
        std::move(this->Second).consumeAAResults(NumVars),
    };
  }
};

template <typename FirstT, typename SecondT>
UnionFindAACombinator(FirstT, SecondT)
    -> UnionFindAACombinator<FirstT, SecondT>;

/// Lazy caching of union-find alias-analysis results. This class is *not*
/// thread-safe!
template <UnionFindAAResult AAResT, bool ShouldCacheMayAlias = true>
class CachedUnionFindAAResult {
public:
  explicit CachedUnionFindAAResult(AAResT &&AARes) : AARes(std::move(AARes)) {
    Var2AliasVars.resize(this->AARes.size());
    ComputedAliasSetFor.reserve(this->AARes.size());
  }

  [[nodiscard]] static constexpr bool isCached() noexcept { return true; }

  [[nodiscard]] constexpr size_t size() const noexcept { return AARes.size(); }

  [[nodiscard]] const RawAliasSet<ValueId> &getRawAliasSet(ValueId Var) const {
    if (ComputedAliasSetFor.tryInsert(Var)) [[unlikely]] {
      Var2AliasVars[Var] = AARes.getRawAliasSet(Var);
    }
    return Var2AliasVars[Var];
  }

  [[nodiscard]] bool mayAlias(ValueId Var1, ValueId Var2) const {
    if constexpr (ShouldCacheMayAlias) {
      return getRawAliasSet(Var1).contains(Var2);
    } else {
      return AARes.mayAlias(Var1, Var2);
    }
  }

private:
  // TODO: Combine Var2AliasVars+ComputedAliasSetFor into a ValueIdMap!
  mutable TypedVector<ValueId, RawAliasSet<ValueId>> Var2AliasVars;
  mutable BitSet<ValueId> ComputedAliasSetFor{};

  [[no_unique_address]] AAResT AARes;
};

/// Implements the AliasIterator interface on union-find alias-analysis results.
template <typename AAResT, typename Var2IdMapper, typename Id2VarMapper>
  requires UnionFindAAResult<std::remove_cvref_t<AAResT>>
struct UnionFindAliasIterator {
  [[no_unique_address]] AAResT AARes;
  [[no_unique_address]] Var2IdMapper Var2Id;
  [[no_unique_address]] Id2VarMapper Id2Var;

  void forallAliasesOf(auto &&Ptr, auto && /*Inst*/, auto Callback) {
    auto Id = std::invoke(Var2Id, Ptr);
    const auto &RawAliases = AARes.getRawAliasSet(Id);
    RawAliases.foreach ([&, Callback = copyOrRef(Callback)](ValueId Alias) {
      std::invoke(Id2Var, Alias, Callback);
    });
  }
};

/// Common base-class of union-find alias-analysis results.
struct UnionFindAAResultBase {
  enum class ObjectRepId : uint32_t {};

  TypedVector<ObjectRepId, RawAliasSet<ValueId>> BackwardView;

  void dump(llvm::raw_ostream &OS = llvm::errs(), uint32_t Indent = 0) const;
};

/// Basic implementation of union-find alias-analysis results.
struct BasicUnionFindAAResult : UnionFindAAResultBase {
  TypedVector<ValueId, ObjectRepId> Var2Rep;

  [[nodiscard]] static constexpr bool isCached() noexcept { return true; }
  [[nodiscard]] constexpr size_t size() const noexcept {
    return Var2Rep.size();
  }

  [[nodiscard]] const RawAliasSet<ValueId> &
  getRawAliasSet(ValueId Var) const noexcept {
    auto Rep = Var2Rep[Var];
    return BackwardView[Rep];
  }

  [[nodiscard]] bool mayAlias(ValueId Var1, ValueId Var2) const noexcept {
    if (Var1 == Var2) {
      return true;
    }
    auto Rep1 = Var2Rep[Var1];
    auto Rep2 = Var2Rep[Var2];
    return Rep1 == Rep2;
  }
};

/// Union-find alias-analysis results for an analysis that achieves (partial)
/// calling-context sensitivity through heap cloning.
struct CallingContextSensUnionFindAAResult : UnionFindAAResultBase {
  TypedVector<ValueId, llvm::SmallVector<ObjectRepId, 2>> Var2Rep{};

  [[nodiscard]] static constexpr bool isCached() noexcept { return false; }
  [[nodiscard]] constexpr size_t size() const noexcept {
    return Var2Rep.size();
  }

  [[nodiscard]] RawAliasSet<ValueId> getRawAliasSet(ValueId Var) const {
    RawAliasSet<ValueId> ResultSet;
    for (auto Rep : Var2Rep[Var]) {
      ResultSet |= BackwardView[Rep];
    }
    return ResultSet;
  }

  [[nodiscard]] bool mayAlias(ValueId Var1, ValueId Var2) const noexcept {
    if (Var1 == Var2) {
      return true;
    }

    // Note: K is mostly small (1 or 2), so below loop should be very cheap
    const auto &Reps1 = Var2Rep[Var1];
    const auto &Reps2 = Var2Rep[Var2];
    for (ObjectRepId R1 : Reps1) {
      if (llvm::is_contained(Reps2, R1)) {
        return true;
      }
    }
    return false;
  }
};

/// Union-find alias-analysis that is neither context, nor field, nor
/// indirection sensitive.
/// Most useful for building a more precise alias-analysis on top of this,
/// instead of using this directly.
///
/// \tparam AnalysisDomainT The analysis domain to use, e.g., LLVMPAGDomain.
/// \tparam ObjectIdT The type of object-Ids (default ValueId).
template <typename AnalysisDomainT, typename ObjectIdT = ValueId>
struct BasicUnionFindAA {
  using ObjectId = ObjectIdT;

  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  void onAddEdge(ObjectId From, ObjectId To, pag::Edge /*E*/,
                 Nullable<n_t> /*AtInstruction*/) {
    AliasSets.join(From, To);
  }

  void onAddValue(ByConstRef<v_t> /*Var*/, ObjectId VId) {
    AliasSets.grow(size_t(VId) + 1);
  }

  template <std::invocable<ValueId> Var2ObjFn = IdentityFn>
  [[nodiscard]] BasicUnionFindAAResult
  consumeAAResults(size_t NumVars, Var2ObjFn Var2Obj = {}) && {
    auto Equiv = std::move(AliasSets)
                     .template compress<BasicUnionFindAAResult::ObjectRepId>();

    static constexpr auto InvalidRep =
        BasicUnionFindAAResult::ObjectRepId(UINT32_MAX);

    BasicUnionFindAAResult Result{};
    Result.BackwardView.resize(Equiv.numClasses());
    Result.Var2Rep.resize(NumVars, InvalidRep);

    for (auto VId : iota<ValueId>(NumVars)) {
      // TODO: Skip only-ret val-ids
      auto ObjId = std::invoke(Var2Obj, VId);
      auto Rep = Equiv[ObjId];
      assert(Result.Var2Rep[VId] == InvalidRep &&
             "Mapping from ValueId to AliasSet-Id must be injective!");
      Result.Var2Rep[VId] = Rep;
      Result.BackwardView[Rep].insert(VId);
    }

    return Result;
  }

  UnionFind<ObjectId> AliasSets{};
};

/// Union-find alias-analysis that achieves (partial) calling-context
/// sensitivity through heap cloning.
/// Implements pag::PBStrategy.
///
/// \tparam AnalysisDomainT The analysis domain to use, e.g., LLVMPAGDomain.
/// \tparam K The maximum length of call-strings (default 1).
template <typename AnalysisDomainT, unsigned K = 1>
class CallingContextSensUnionFindAA {
public:
  enum class CtxObjectId : uint32_t {};

  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  constexpr CallingContextSensUnionFindAA(
      NonNullPtr<const CallGraph<n_t, f_t>> CG,
      NonNullPtr<const db_t> IRDB) noexcept
      : CG(CG), IRDB(IRDB) {}

  void onAddEdge(ValueId From, ValueId To, pag::Edge E,
                 Nullable<n_t> CallSite) {
    // llvm::errs() << "[CtxAA][onAddEdge]: From: " << int(From)
    //              << ", To: " << int(To) << ", E: " << to_string(E)
    //              << ", At: " << llvmIRToString(CallSite) << '\n';
    E.apply(Overloaded{
        [&, this, CallSite](pag::Call) {
          for (const auto &[ArgCtxId, FromObj] : Var2Obj[From]) {
            auto ParamCtx = CC[ArgCtxId].withPrefix(CallSite);
            auto ParamCtxId = CC.getOrInsert(ParamCtx);

            if (const auto *Dst = getObjectOrNull(To, ParamCtxId)) {
              Base.AliasSets.join(FromObj, *Dst);
            }
          }
        },
        [&, this, CallSite](pag::Return) {
          for (const auto &[CSValCtxId, ToObj] : Var2Obj[To]) {
            auto RetCtx = CC[CSValCtxId].withPrefix(CallSite);
            auto RetCtxId = CC.getOrInsert(RetCtx);

            if (const auto *Src = getObjectOrNull(From, RetCtxId)) {
              Base.AliasSets.join(*Src, ToObj);
            }
          }
        },
        [&, this](auto) {
          const auto &FromObjects = Var2Obj[From];
          for (const auto &[FromCtx, FromObj] : FromObjects) {
            if (FromCtx == CallingContextId::None) {
              for (const auto &[ToCtx, ToObj] : Var2Obj[To]) {
                Base.AliasSets.join(FromObj, ToObj);
              }
            } else if (const auto *Dst = getObjectOrNull(To, FromCtx)) {
              Base.AliasSets.join(FromObj, *Dst);
            }
          }
        },
    });
  }

  void onAddValue(ByConstRef<v_t> Var, ValueId VId) {
    Var2Obj.emplace_back();
    if (const auto &Fun = getFunction(Var)) {
      CC.visitAllCallingContexts(
          unwrapNullable(Fun), *CG, *IRDB,
          [this, VId](ByConstRef<n_t> /*FirstCS*/, CallingContextId Ctx) {
            std::ignore = getObject(VId, Ctx);
          });

      if (!CG->getCallersOf(unwrapNullable(Fun)).empty()) {
        return;
      }
      // fallback in case, we have no callers
    }
    std::ignore = getObject(VId, CallingContextId::None);
  }

  void withCalleesOfCallAt(ByConstRef<n_t> CS,
                           std::invocable<f_t> auto WithCallee) const {
    for (const auto &Callee : CG->getCalleesOfCallAt(CS)) {
      std::invoke(WithCallee, Callee);
    }
  }

  /// Retrieve the analysis results after buildPAG() has returned.
  [[nodiscard]] CallingContextSensUnionFindAAResult
  consumeAAResults(size_t NumVars) && {
    auto Equiv = std::move(Base.AliasSets)
                     .template compress<BasicUnionFindAAResult::ObjectRepId>();

    CallingContextSensUnionFindAAResult Result{};
    Result.BackwardView.resize(Equiv.numClasses());
    Result.Var2Rep.resize(NumVars);

    for (const auto &[ValId, Objects] : Var2Obj.enumerate()) {
      // TODO: Skip only-ret val-ids

      for (const auto &[_, Obj] : Objects) {
        auto Rep = Equiv[Obj];
        if (Result.BackwardView[Rep].tryInsert(ValId)) {
          Result.Var2Rep[ValId].push_back(Rep);
        }
      }
      if (Objects.empty()) {
        llvm::errs() << "Warning: Empty Objects-set for Var#" << uint32_t(ValId)
                     << '\n';
      }
    }

    return Result;
  }

private:
  [[nodiscard]] Nullable<f_t> getFunction(ByConstRef<v_t> Var) {
    if constexpr (requires() { getPointerFrom(Var)->getFunction(); }) {
      return getPointerFrom(Var)->getFunction();
    } else {
      return IRDB->getFunctionOf(Var);
    }
  }

  [[nodiscard]] const CtxObjectId *getObjectOrNull(ValueId Val,
                                                   CallingContextId Ctx) {

    const auto *Ret = getOrNull(Var2Obj[Val], Ctx);
    if (!Ret) {
      Ret = getOrNull(Var2Obj[Val], CallingContextId::None);
    }

    return Ret;
  }
  [[nodiscard]] CtxObjectId getObject(ValueId VId, CallingContextId Ctx) {
    auto [It, Inserted] =
        Var2Obj[VId].try_emplace(Ctx, CtxObjectId(Obj2Var.size()));
    if (Inserted) {
      Obj2Var.emplace_back(VId, Ctx);
      Base.AliasSets.grow(size_t(It->second) + 1);
    }
    return It->second;
  }

  NonNullPtr<const CallGraph<n_t, f_t>> CG;
  NonNullPtr<const db_t> IRDB;
  TypedVector<CtxObjectId, std::pair<ValueId, CallingContextId>> Obj2Var{};
  TypedVector<ValueId, llvm::SmallDenseMap<CallingContextId, CtxObjectId>>
      Var2Obj{};

  CallingContextConstructor<n_t, f_t, K> CC{};
  BasicUnionFindAA<AnalysisDomainT, CtxObjectId> Base{};
};

/// Union-find alias-analysis that achieves (partial) indirection-depth-context
/// sensitivity through heap cloning.
/// Use this with pag::PBMixin to implement pag::PBStrategy.
///
/// \tparam AnalysisDomainT The analysis domain to use, e.g., LLVMPAGDomain.
/// \tparam K The maximum depth of pointer-indirections that can be
/// differentiated (default 5).
template <typename AnalysisDomainT, unsigned K = 5>
class IndirectionSensUnionFindAA {
public:
  static_assert(K > 0, "A depth-limit of 0 is invalid!");

  enum class IndObjectId : uint32_t {};
  enum class IndDepth : uint32_t {};

  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  void onAddEdge(ValueId From, ValueId To, pag::Edge E,
                 Nullable<n_t> /*CallSite*/) {
    using namespace pag;
    const auto &FromFldObjects = Var2Obj[From];
    for (const auto &[FromCtx, FromObj] : FromFldObjects.enumerate()) {
      switch (E.kind()) {
      case Edge::kindOf<Copy>():
        if (FromCtx == IndDepth{}) {
          break;
        }

      // fallthrough
      case Edge::kindOf<Assign>():
      case Edge::kindOf<Gep>():
      case Edge::kindOf<Call>():
      case Edge::kindOf<Return>():
        if (auto ToObj = getObjectOrNull(To, FromCtx)) {
          Base.AliasSets.join(FromObj, *ToObj);
        }

        break;
      case Edge::kindOf<Load>():
        if (auto ToCtx = prevDepth(FromCtx)) {
          if (auto ToObj = getObjectOrNull(To, *ToCtx)) {
            Base.AliasSets.join(FromObj, *ToObj);
          }
        }
        if (FromCtx == IndDepth(K - 1)) {
          // For soundness
          if (auto ToObj = getObjectOrNull(To, FromCtx)) {
            Base.AliasSets.join(FromObj, *ToObj);
          }
        }
        break;
      case Edge::kindOf<Store>():
      case Edge::kindOf<StorePOI>():
        if (auto ToObj = getObjectOrNull(To, nextDepth(FromCtx))) {
          Base.AliasSets.join(FromObj, *ToObj);
        }

        break;
      default:
        llvm_unreachable("This switch should cover all cases explicitly");
      }
    }
  }

  void onAddValue(ByConstRef<v_t> /*Var*/, ValueId VId) {
    auto Obj = Obj2Var.size();
    Base.AliasSets.grow(Obj + K);
    Var2Obj.emplace_back(generate_tag, [Obj](IndDepth Depth) {
      return IndObjectId(Obj + size_t(Depth));
    });
    // TODO: For some values, we can already see that depth>=2 does not make
    // sense, so we could exit the below loop early
    for (auto Depth : iota<IndDepth>(K)) {
      // Var2Obj[VId].push_back(IndObjectId(Obj));
      // Obj++;
      Obj2Var.emplace_back(VId, Depth);
    }
  }

  /// Retrieve the analysis results after buildPAG() has returned.
  [[nodiscard]] BasicUnionFindAAResult consumeAAResults(size_t NumVars) && {
    return std::move(Base).consumeAAResults(NumVars, [this](ValueId VId) {
      // When presenting the results, we only care about the values at depth 0
      return Var2Obj[VId][IndDepth{}];
    });
  }

private:
  [[nodiscard]] std::optional<IndObjectId> getObjectOrNull(ValueId VId,
                                                           IndDepth Ctx) {
    const auto &Map = Var2Obj[VId];
    if (!Map.inbounds(Ctx)) {
      return std::nullopt;
    }

    return Map[Ctx];
  }

  constexpr static IndDepth nextDepth(IndDepth Ctx) noexcept {
    if (Ctx == IndDepth(K - 1)) {
      return Ctx;
    }

    return IndDepth(uint32_t(Ctx) + 1);
  }
  constexpr static std::optional<IndDepth> prevDepth(IndDepth Ctx) noexcept {
    if (Ctx == IndDepth{}) {
      return std::nullopt;
    }
    return IndDepth(uint32_t(Ctx) - 1);
  }

  TypedVector<ValueId, TypedArray<IndDepth, IndObjectId, K>> Var2Obj{};
  TypedVector<IndObjectId, std::pair<ValueId, IndDepth>> Obj2Var{};
  BasicUnionFindAA<AnalysisDomainT, IndObjectId> Base{};
};
} // namespace psr
