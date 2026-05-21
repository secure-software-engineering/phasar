#pragma once

#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

PHASAR_STRONG_TYPEDEF(psr::wpds, uint32_t, ESGNodeId);

namespace psr::wpds {
namespace detail {

template <typename CLTy, typename SETy, typename WeightTy>
struct SolverResultsData {
  using cl_t = CLTy;
  using se_t = SETy;
  using weight_t = WeightTy;

  Compressor<std::pair<cl_t, se_t>, wpds::ESGNodeId> ESGNodeCompressor;
  llvm::DenseMap<std::pair<cl_t, se_t>,
                 llvm::SmallDenseMap<wpds::ESGNodeId, weight_t>>
      PathEdges;
  TypedVector<wpds::ESGNodeId,
              llvm::SmallDenseMap<std::pair<wpds::ESGNodeId, se_t>, weight_t>>
      Incoming;
};

template <typename Derived, typename CLTy, typename SETy, typename WeightTy>
class SolverResultsBase {
public:
  using cl_t = CLTy;
  using se_t = SETy;
  using weight_t = WeightTy;

  [[nodiscard]] bool holdsFactAt(ByConstRef<cl_t> Fact,
                                 ByConstRef<se_t> At) const {
    return holdsFactAt({Fact, At});
  }

  [[nodiscard]] bool
  holdsFactAt(ByConstRef<std::pair<cl_t, se_t>> FactAt) const {
    for (const auto &[Src, Weight] : getOrDefault(self().pathEdges(), FactAt)) {
      if (self().incoming()[Src].empty()) {
        // Directly generated from zero
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] bool isInResultSet(ByConstRef<cl_t> Fact,
                                   ByConstRef<se_t> At) const {
    return isInResultSet({Fact, At});
  }

  [[nodiscard]] bool
  isInResultSet(ByConstRef<std::pair<cl_t, se_t>> FactAt) const {
    return self().pathEdges().count(FactAt);
  }

  bool traversePath(
      ByConstRef<std::pair<cl_t, se_t>> FactAt,
      std::invocable<const cl_t &, const se_t &, const cl_t &, const se_t &,
                     const weight_t &> auto WithPATransition) const {
    llvm::SmallVector<wpds::ESGNodeId> WL;
    llvm::DenseSet<wpds::ESGNodeId> Seen;

    for (const auto &[Src, Weight] : getOrDefault(self().pathEdges(), FactAt)) {
      WL.emplace_back(Src);
      Seen.insert(Src);

      const auto &[SrcCL, SrcSE] = self().esgNodeCompressor()[Src];
      std::invoke(WithPATransition, FactAt.first, FactAt.second, SrcCL, SrcSE,
                  Weight);
    }

    if (WL.empty()) {
      // Fact does not hold
      return false;
    }

    do {
      auto Curr = WL.pop_back_val();
      const auto &[CurrCL, CurrSE] = self().esgNodeCompressor()[Curr];

      const auto &Inc = self().incoming()[Curr];
      if (!Inc.empty()) {
        for (const auto &[CSNode, Weight] : Inc) {
          const auto &[CSSrc, RetSite] = CSNode;

          const auto &[CSSrcCL, CSSrcSE] = self().esgNodeCompressor()[CSSrc];
          std::invoke(WithPATransition, CurrCL, CurrSE, CSSrcCL, CSSrcSE,
                      Weight);

          if (Seen.insert(CSSrc).second) {
            WL.emplace_back(CSSrc);
          }
        }
      }
    } while (!WL.empty());

    return true;
  }

  [[nodiscard]] auto getAllIFDSResultEntries() const {
    return llvm::make_filter_range(self().esgNodeCompressor(),
                                   [this](const auto &Entry) {
                                     const auto &[CL, SE] = Entry;
                                     return this->isInResultSet(CL, SE);
                                   });
  }

  void dumpPath(ByConstRef<cl_t> Fact, ByConstRef<se_t> At,
                llvm::raw_ostream &OS);

  void dumpResults(llvm::raw_ostream &OS = llvm::outs());

private:
  constexpr SolverResultsBase() noexcept = default;
  friend Derived;

  [[nodiscard]] const auto &self() const noexcept {
    return *static_cast<const Derived *>(this);
  }
};

template <typename Derived, typename CLTy, typename SETy, typename WeightTy>
void SolverResultsBase<Derived, CLTy, SETy, WeightTy>::dumpPath(
    ByConstRef<cl_t> Fact, ByConstRef<se_t> At, llvm::raw_ostream &OS) {
  OS << "digraph Path {\n";

  Compressor<std::pair<cl_t, int>> CLC;
  const auto AddCL = [&](cl_t CL) -> uint32_t {
    auto [Id, Inserted] = CLC.insert(std::pair{CL, false});
    Id++;
    if (Inserted) {
      OS << "  " << Id << "[label=\"";
      OS.write_escaped(DToString(CL)) << "\"];\n";
    }
    return Id;
  };

  const auto AddSrcCL = [&](cl_t CL) -> uint32_t {
    auto [Id, Inserted] = CLC.insert(std::pair{CL, true});
    Id++;
    if (Inserted) {
      OS << "  " << Id << "[label=\"q[";
      OS.write_escaped(DToString(CL)) << "]\", style=dashed];\n";
    }
    return Id;
  };

  traversePath({Fact, At},
               [&](const auto &TgtCL, const auto &TgtSE, const auto &SrcCL,
                   const auto & /*SrcSE*/, const auto & /*Weight*/) {
                 auto CurrCLId = AddCL(TgtCL);
                 auto SrcCLId = AddSrcCL(SrcCL);

                 OS << "  " << CurrCLId << "->" << SrcCLId << "[label=\"";
                 OS.write_escaped(NToString(TgtSE)) << "\"];\n";
               });

  OS << "}\n";
}

template <typename Derived, typename CLTy, typename SETy, typename WeightTy>
void SolverResultsBase<Derived, CLTy, SETy, WeightTy>::dumpResults(
    llvm::raw_ostream &OS) {
  OS << "digraph RuleBasedSolver {\n";
  OS << "  rankdir=LR;\n";
  OS << "  node [shape=circle];\n";
  OS << "  0[label=\"ACC\", shape=doublecircle];\n";

  Compressor<std::pair<cl_t, int>> CLC;
  const auto AddCL = [&](cl_t CL) -> uint32_t {
    auto [Id, Inserted] = CLC.insert(std::pair{CL, false});
    Id++;
    if (Inserted) {
      OS << "  " << Id << "[label=\"";
      OS.write_escaped(DToString(CL)) << "\"];\n";
    }
    return Id;
  };

  const auto AddSrcCL = [&](cl_t CL) -> uint32_t {
    auto [Id, Inserted] = CLC.insert(std::pair{CL, true});
    Id++;
    if (Inserted) {
      OS << "  " << Id << "[label=\"q[";
      OS.write_escaped(DToString(CL)) << "]\", style=dashed];\n";
    }
    return Id;
  };
  for (const auto &[PE, Inner] : self().pathEdges()) {
    const auto &[TgtCL, TgtSE] = PE;
    auto TgtClId = AddCL(TgtCL);

    for (const auto &[Src, Weight] : Inner) {
      const auto &[SrcCL, SrcSE] = self().esgNodeCompressor()[Src];
      auto SrcClId = AddSrcCL(SrcCL);

      OS << "  " << TgtClId << "->" << SrcClId << "[label=\"";
      OS.write_escaped(NToString(TgtSE)) << "\"];\n";

      if (SrcSE == TgtSE && SrcCL == TgtCL) {
        const auto &Inc = self().incoming()[Src];
        if (!Inc.empty()) {
          for (const auto &[CSNode, _] : Inc) {
            const auto &[CSSrc, RetSite] = CSNode;
            const auto &CSSrcFact = self().esgNodeCompressor()[CSSrc].first;

            auto CSClId = AddSrcCL(CSSrcFact);

            OS << "  " << SrcClId << "->" << CSClId << "[label=\"";
            OS.write_escaped(NToString(SrcSE)) << "\", style=dashed];\n";
          }
        } else {
          OS << "  " << SrcClId << "->0[label=\"";
          OS.write_escaped(NToString(SrcSE)) << "\", style=dashed];\n";
        }
        continue;
      }
    }
  }

  OS << "}\n";
}
} // namespace detail

template <typename CLTy, typename SETy, typename WeightTy>
class OwningSolverResults
    : public detail::SolverResultsBase<
          OwningSolverResults<CLTy, SETy, WeightTy>, CLTy, SETy, WeightTy> {
  friend detail::SolverResultsBase<OwningSolverResults<CLTy, SETy, WeightTy>,
                                   CLTy, SETy, WeightTy>;

public:
  OwningSolverResults(detail::SolverResultsData<CLTy, SETy, WeightTy> &&Data)
      : Data(std::move(Data)) {}

private:
  [[nodiscard]] const auto &pathEdges() const noexcept {
    return Data.PathEdges;
  }
  [[nodiscard]] const auto &incoming() const noexcept { return Data.Incoming; }
  [[nodiscard]] const auto &esgNodeCompressor() const noexcept {
    return Data.ESGNodeCompressor;
  }

  detail::SolverResultsData<CLTy, SETy, WeightTy> Data;
};

template <typename CLTy, typename SETy, typename WeightTy>
class SolverResults
    : public detail::SolverResultsBase<SolverResults<CLTy, SETy, WeightTy>,
                                       CLTy, SETy, WeightTy> {
  friend detail::SolverResultsBase<SolverResults<CLTy, SETy, WeightTy>, CLTy,
                                   SETy, WeightTy>;

public:
  constexpr SolverResults(
      NonNullPtr<const detail::SolverResultsData<CLTy, SETy, WeightTy>>
          Data) noexcept
      : Data(Data) {}

private:
  [[nodiscard]] const auto &pathEdges() const noexcept {
    return Data->PathEdges;
  }
  [[nodiscard]] const auto &incoming() const noexcept { return Data->Incoming; }
  [[nodiscard]] const auto &esgNodeCompressor() const noexcept {
    return Data->ESGNodeCompressor;
  }

  NonNullPtr<const detail::SolverResultsData<CLTy, SETy, WeightTy>> Data;
};

} // namespace psr::wpds
