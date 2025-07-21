#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "SourceMapping.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <set>
#include <tuple>
#include <utility>
#include <variant>

namespace psr {

struct GlobalVar {
  llvm::StringRef Name;

  friend bool operator<(GlobalVar G1, GlobalVar G2) noexcept {
    return G1.Name < G2.Name;
  }
  friend bool operator==(GlobalVar G1, GlobalVar G2) noexcept {
    return G1.Name == G2.Name;
  }
};
struct LineCol {
  uint32_t Line{};
  uint32_t Col{};

  friend bool operator<(LineCol LC1, LineCol LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col) < std::tie(LC2.Line, LC2.Col);
  }
  friend bool operator==(LineCol LC1, LineCol LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col) == std::tie(LC2.Line, LC2.Col);
  }
};
struct LineColFun {
  uint32_t Line{};
  uint32_t Col{};
  llvm::StringRef InFunction{};

  friend bool operator<(LineColFun LC1, LineColFun LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col, LC1.InFunction) <
           std::tie(LC2.Line, LC2.Col, LC2.InFunction);
  }
  friend bool operator==(LineColFun LC1, LineColFun LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col, LC1.InFunction) ==
           std::tie(LC2.Line, LC2.Col, LC2.InFunction);
  }
};
struct ArgNo {
  uint32_t Idx{};

  friend bool operator<(ArgNo A1, ArgNo A2) noexcept { return A1.Idx < A2.Idx; }
  friend bool operator==(ArgNo A1, ArgNo A2) noexcept {
    return A1.Idx == A2.Idx;
  }
};
struct ArgInFun {
  uint32_t Idx;
  llvm::StringRef InFunction{};

  friend bool operator<(ArgInFun A1, ArgInFun A2) noexcept {
    return std::tie(A1.Idx, A1.InFunction) < std::tie(A2.Idx, A2.InFunction);
  }
  friend bool operator==(ArgInFun A1, ArgInFun A2) noexcept {
    return std::tie(A1.Idx, A1.InFunction) == std::tie(A2.Idx, A2.InFunction);
  }
};

struct TestingSrcLocation
    : public std::variant<LineCol, LineColFun, GlobalVar, ArgNo, ArgInFun> {

  using std::variant<LineCol, LineColFun, GlobalVar, ArgNo, ArgInFun>::variant;

  template <typename T> [[nodiscard]] constexpr bool isa() const noexcept {
    return std::holds_alternative<T>(*this);
  }
  template <typename T>
  [[nodiscard]] constexpr const T *dyn_cast() const noexcept {
    return std::get_if<T>(this);
  }
  template <typename T> [[nodiscard]] constexpr T *dyn_cast() noexcept {
    return std::get_if<T>(this);
  }
};

[[nodiscard]] inline const llvm::Value *
testingLocInIR(TestingSrcLocation Loc, const LLVMProjectIRDB &IRDB,
               const llvm::Function *InterestingFunction = nullptr) {

  return std::visit(
      psr::Overloaded{
          [=](LineCol LC) -> llvm ::Value const * {
            if (!InterestingFunction) {
              llvm::report_fatal_error(
                  "You must provide an InterestingFunction as last parameter "
                  "to testingLocInIR(), if trying to resolve a LineCol; "
                  "alternatively use LineColFun instead.");
            }

            return unittest::getInstAtOrNull(InterestingFunction, LC.Line,
                                             LC.Col);
          },
          [&IRDB](LineColFun LC) -> llvm ::Value const * {
            const auto *InFun = IRDB.getFunctionDefinition(LC.InFunction);
            if (!InFun) {
              llvm::report_fatal_error("Required function '" + LC.InFunction +
                                       "' does not exist in the IR!");
            }
            return unittest::getInstAtOrNull(InFun, LC.Line, LC.Col);
          },
          [&IRDB](GlobalVar GV) -> llvm ::Value const * {
            return IRDB.getModule()->getGlobalVariable(GV.Name, true);
          },
          [=](ArgNo A) -> llvm ::Value const * {
            if (!InterestingFunction) {
              llvm::report_fatal_error(
                  "You must provide an InterestingFunction as last parameter "
                  "to testingLocInIR(), if trying to resolve an ArgNo; "
                  "alternatively use ArgInFun instead.");
            }
            if (InterestingFunction->arg_size() <= A.Idx) {
              llvm::report_fatal_error(
                  "Argument index " + llvm::Twine(A.Idx) +
                  " is out of range (" +
                  llvm::Twine(InterestingFunction->arg_size()) + ")!");
            }
            return InterestingFunction->getArg(A.Idx);
          },
          [&IRDB](ArgInFun A) -> llvm ::Value const * {
            const auto *InFun = IRDB.getFunctionDefinition(A.InFunction);
            if (!InFun) {
              llvm::report_fatal_error("Required function '" + A.InFunction +
                                       "' does not exist in the IR!");
            }
            if (InFun->arg_size() <= A.Idx) {
              llvm::report_fatal_error("Argument index " + llvm::Twine(A.Idx) +
                                       " is out of range (" +
                                       llvm::Twine(InFun->arg_size()) + ")!");
            }
            return InFun->getArg(A.Idx);
          },
      },
      Loc);
}

template <typename SetTy>
[[nodiscard]] inline std::set<const llvm::Value *>
convertTestingLocationSetInIR(
    const SetTy &Locs, const LLVMProjectIRDB &IRDB,
    const llvm::Function *InterestingFunction = nullptr) {
  std::set<const llvm::Value *> Ret;
  llvm::transform(Locs, std::inserter(Ret, Ret.end()),
                  [&](TestingSrcLocation Loc) {
                    return testingLocInIR(Loc, IRDB, InterestingFunction);
                  });
  return Ret;
}

struct SrcCodeLocationEntry {
  // TODO: the const llvm::Instruction * variant is not good, as it basically
  // completely ignores the line and column aspect of the source file. We want
  // to tie the unittest to the source file, but for very specific cases, this
  // variant was the only way I found to make the unittests run.
  SrcCodeLocationEntry(
      uint32_t Line, uint32_t Column,
      std::variant<const llvm::Function *, const llvm::GlobalVariable *,
                   const llvm::Instruction *>
          Context)
      : Line(Line), Column(Column), Context(Context) {}
  SrcCodeLocationEntry(
      uint32_t Line, uint32_t Column,
      std::variant<const llvm::Function *, const llvm::GlobalVariable *,
                   const llvm::Instruction *>
          Context,
      std::function<bool(const llvm::Instruction *Inst)> LambdaFunc)
      : Line(Line), Column(Column), LambdaFunc(std::move(LambdaFunc)),
        Context(Context) {}
  uint32_t Line = 0;
  uint32_t Column = 0;
  std::function<bool(const llvm::Instruction *Inst)> LambdaFunc = nullptr;
  std::variant<const llvm::Function *, const llvm::GlobalVariable *,
               const llvm::Instruction *>
      Context;

  bool operator==(const SrcCodeLocationEntry &Other) const {
    return Line == Other.Line && Column == Other.Column;
  }
  bool operator<(const SrcCodeLocationEntry &Other) const {
    return std::tie(Line, Column) < std::tie(Other.Line, Other.Column);
  }
};

inline std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
getGroundTruthInsts(
    const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
        &GroundTruth) {
  std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
      GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    const auto &FirstEntry = std::get<0>(Entry);
    const auto &SecondEntry = std::get<1>(Entry);
    const llvm::Instruction *CurrInst = nullptr;
    const llvm::Value *CurrVal = nullptr;

    if (std::get_if<const llvm::GlobalVariable *>(&FirstEntry.Context)) {
      llvm::report_fatal_error("Cannot cast global variable to Instruction\n");
    }

    if (const auto *Inst =
            std::get_if<const llvm::Instruction *>(&FirstEntry.Context)) {
      if (*Inst) {
        CurrInst = *Inst;
      } else {
        llvm::report_fatal_error("Given Ground Truth Instruction was null.\n");
      }
    } else if (const auto *Func =
                   std::get_if<const llvm::Function *>(&FirstEntry.Context)) {
      if (FirstEntry.LambdaFunc) {
        CurrInst = unittest::getInstAtOrNull(
            *Func, FirstEntry.Line, FirstEntry.Column, FirstEntry.LambdaFunc);
      } else {
        CurrInst = unittest::getInstAtOrNull(*Func, FirstEntry.Line,
                                             FirstEntry.Column);
      }
    } else {
      llvm::report_fatal_error("Unknown variant type.\n");
    }

    if (const auto *GlobalVar =
            std::get_if<const llvm::GlobalVariable *>(&SecondEntry.Context)) {
      CurrVal = llvm::cast<llvm::Value>(*GlobalVar);
    } else if (const auto *Func =
                   std::get_if<const llvm::Function *>(&FirstEntry.Context)) {
      const llvm::Instruction *AsInst = nullptr;
      if (SecondEntry.LambdaFunc) {
        AsInst = unittest::getInstAtOrNull(*Func, SecondEntry.Line,
                                           SecondEntry.Column,
                                           SecondEntry.LambdaFunc);
      } else {
        AsInst = unittest::getInstAtOrNull(*Func, SecondEntry.Line,
                                           SecondEntry.Column);
      }
      if (const auto *CanBeCastToValue =
              llvm::dyn_cast_or_null<llvm::Value>(AsInst)) {
        CurrVal = CanBeCastToValue;
      } else {
        llvm::errs() << "AsInst Instruction couldn't be cast to value.\n";
      }
    } else {
      llvm::report_fatal_error("Unknown variant type.\n");
    }

    if (CurrInst) {
      if (CurrVal) {
        GroundTruthEntries.insert({CurrInst, CurrVal});
      }
    }
  }

  return GroundTruthEntries;
};

inline std::set<const llvm::Instruction *>
getGroundTruthInsts(const std::set<SrcCodeLocationEntry> &GroundTruth) {
  std::set<const llvm::Instruction *> GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    if (std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
      llvm::report_fatal_error("Cannot cast global variable to Instruction\n");
    }

    if (const auto *Inst =
            std::get_if<const llvm::Instruction *>(&Entry.Context)) {
      if (*Inst) {
        GroundTruthEntries.insert(*Inst);
        continue;
      }

      llvm::report_fatal_error("Given Ground Truth Instruction was null.\n");
    }

    if (const auto *Func =
            std::get_if<const llvm::Function *>(&Entry.Context)) {
      if (Entry.LambdaFunc) {
        GroundTruthEntries.insert(unittest::getInstAtOrNull(
            *Func, Entry.Line, Entry.Column, Entry.LambdaFunc));
      } else {
        GroundTruthEntries.insert(
            unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column));
      }
      continue;
    }
    llvm::report_fatal_error("Unknown variant type.\n");
  }

  return GroundTruthEntries;
};

inline std::set<const llvm::Value *>
getGroundTruthValues(const std::set<SrcCodeLocationEntry> &GroundTruth) {
  std::set<const llvm::Value *> GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    if (const auto *Func =
            std::get_if<const llvm::Function *>(&Entry.Context)) {
      if (Entry.LambdaFunc) {
        if (const auto *FuncVariantInst = unittest::getInstAtOrNull(
                *Func, Entry.Line, Entry.Column, Entry.LambdaFunc)) {
          if (const auto *CurrVal =
                  llvm::dyn_cast_or_null<llvm::Value>(FuncVariantInst)) {
            GroundTruthEntries.insert(CurrVal);
            continue;
          }
          llvm::errs()
              << "FuncVariantInst Instruction couldn't be cast to value.\n";
        }

        continue;
      }

      if (const auto *FuncVariantInst =
              unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column)) {
        llvm::outs() << "FuncVariantInst: " << FuncVariantInst << "\n";
        llvm::outs() << "*FuncVariantInst: " << *FuncVariantInst << "\n";
        if (const auto *CurrVal =
                llvm::dyn_cast_or_null<llvm::Value>(FuncVariantInst)) {
          GroundTruthEntries.insert(CurrVal);
          continue;
        }

        llvm::errs()
            << "FuncVariantInst Instruction couldn't be cast to value.\n";
      } else {
        llvm::errs() << "getInstAtOrNull returned null\n";
        llvm::errs() << "Entry.Line:   " << Entry.Line << "\n";
        llvm::errs() << "Entry.Column: " << Entry.Column << "\n";
        llvm::errs() << "*Func: " << *Func << "\n";
      }

      continue;
    }

    if (const auto *GlobalVar =
            std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
      GroundTruthEntries.insert(llvm::cast<llvm::Value>(*GlobalVar));
      continue;
    }
    if (const auto *Inst =
            std::get_if<const llvm::Instruction *>(&Entry.Context)) {
      if (*Inst) {
        GroundTruthEntries.insert(llvm::cast<llvm::Value>(*Inst));
        continue;
      }

      llvm::report_fatal_error("Given Ground Truth Instruction was null.\n");
    }

    llvm::report_fatal_error("Unknown variant type.\n");
  }

  return GroundTruthEntries;
};

} // namespace psr

#endif
