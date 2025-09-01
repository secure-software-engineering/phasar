#ifndef PHASAR_PHASARLLVM_UTILS_LLVMANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_LLVMANALYSISPRINTER_H

#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/DefaultAnalysisPrinter.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"

#include <type_traits>

namespace psr {

template <typename AnalysisDomainTy>
class DefaultLLVMAnalysisPrinter
    : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  DefaultLLVMAnalysisPrinter() noexcept = default;

private:
  static std::optional<DebugLocation> getDbgLoc(n_t Instr,
                                                ByConstRef<d_t> DfFact) {
    auto Ret = getDebugLocation(Instr);
    if constexpr (std::is_convertible_v<ByConstRef<d_t>, const llvm::Value *>) {
      if (!Ret) {
        Ret = getDebugLocation(static_cast<const llvm::Value *>(DfFact));
      }
    }
    return Ret;
  }

  void printVariables(llvm::raw_ostream &OS,
                      llvm::ArrayRef<Warning<AnalysisDomainTy>> Results) {
    if constexpr (std::is_convertible_v<const d_t &, const llvm::Value *>) {
      bool HasVariable = false;
      for (const auto &Warn : Results) {
        if (auto VarName = getVarNameFromIR(Warn.Fact); !VarName.empty()) {
          if (!HasVariable) {
            HasVariable = true;
            OS << "  Variable: " << VarName;
          } else {
            OS << ", " << VarName;
          }
        }
      }
      if (HasVariable) {
        OS << '\n';
      }
    }
  }

  void doOnResult(n_t Instr, d_t DfFact, l_t Lattice,
                  DataFlowAnalysisType AnalysisType) override {
    if (auto DbgLoc = getDbgLoc(Instr, DfFact)) {
      DbgResultsEntries[*DbgLoc].emplace_back(Instr, std::move(DfFact),
                                              std::move(Lattice), AnalysisType);
    } else {
      NonDbgResultsEntries.emplace_back(Instr, std::move(DfFact),
                                        std::move(Lattice), AnalysisType);
    }
  }

  void doOnFinalize(llvm::raw_ostream &OS) override {
    size_t Ctr = 0;
    for (const auto &[DbgLoc, Results] : DbgResultsEntries) {
      OS << '#' << ++Ctr << ": ";
      OS << "At " << getFilePathFromIR(DbgLoc.File) << ':' << DbgLoc.Line << ':'
         << DbgLoc.Column << ":\n";
      OS << "  In Function: " << Results.front().Instr->getFunction()->getName()
         << '\n';
      if (auto SrcCode = getSrcCodeFromIR(DbgLoc); !SrcCode.empty()) {
        OS << "  Source code: " << SrcCode << '\n';
      }
      printVariables(OS, Results);
      OS << "\n  Corresponding IR Statements:\n";
      for (const auto &Warn : Results) {
        OS << "    At IR statement: " << NToString(Warn.Instr) << '\n';
        OS << "      Fact: " << DToString(Warn.Fact) << '\n';

        if constexpr (!std::is_same_v<l_t, BinaryDomain>) {
          OS << "      Value: " << LToString(Warn.LatticeElement) << '\n';
        }
        OS << '\n';
      }
    }

    // Fallback in case we don't have debug information:
    for (const auto &Iter : NonDbgResultsEntries) {
      OS << "At IR statement: " << NToString(Iter.Instr) << '\n';
      OS << "  In Function: " << Iter.Instr->getFunction()->getName() << '\n';
      OS << "  Fact: " << DToString(Iter.Fact) << '\n';

      if constexpr (!std::is_same_v<l_t, BinaryDomain>) {
        OS << "  Value: " << LToString(Iter.LatticeElement) << '\n';
      }
      OS << '\n';
    }

    DbgResultsEntries.clear();
    NonDbgResultsEntries.clear();
  }

  std::map<DebugLocation, llvm::SmallVector<Warning<AnalysisDomainTy>, 1>>
      DbgResultsEntries;
  std::vector<Warning<AnalysisDomainTy>> NonDbgResultsEntries{};
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_LLVMANALYSISPRINTER_H
