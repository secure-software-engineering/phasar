#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

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
#include <set>
#include <utility>
#include <variant>

namespace psr {

struct SrcCodeLocationEntry {
  SrcCodeLocationEntry(
      uint32_t Line, uint32_t Column,
      std::variant<const llvm::Function *, const llvm::GlobalVariable *>
          Context)
      : Line(Line), Column(Column), Context(Context) {}
  SrcCodeLocationEntry(
      uint32_t Line, uint32_t Column,
      std::variant<const llvm::Function *, const llvm::GlobalVariable *>
          Context,
      std::function<bool(const llvm::Instruction *Inst)> LambdaFunc)
      : Line(Line), Column(Column), LambdaFunc(std::move(LambdaFunc)),
        Context(Context) {}
  uint32_t Line = 0;
  uint32_t Column = 0;
  std::function<bool(const llvm::Instruction *Inst)> LambdaFunc = nullptr;
  std::variant<const llvm::Function *, const llvm::GlobalVariable *> Context;

  bool operator==(const SrcCodeLocationEntry &Other) const {
    return Line == Other.Line && Column == Other.Column;
  }
  bool operator<(const SrcCodeLocationEntry &Other) const {
    return std::tie(Line, Column) < std::tie(Other.Line, Other.Column);
  }
};

static std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
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

static std::set<const llvm::Instruction *>
getGroundTruthInsts(const std::set<SrcCodeLocationEntry> &GroundTruth) {
  std::set<const llvm::Instruction *> GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    if (std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
      llvm::report_fatal_error("Cannot cast global variable to Instruction\n");
    } else if (const auto *Func =
                   std::get_if<const llvm::Function *>(&Entry.Context)) {
      if (Entry.LambdaFunc) {
        GroundTruthEntries.insert(unittest::getInstAtOrNull(
            *Func, Entry.Line, Entry.Column, Entry.LambdaFunc));
      } else {
        GroundTruthEntries.insert(
            unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column));
      }
    } else {
      llvm::report_fatal_error("Unknown variant type.\n");
    }
  }

  return GroundTruthEntries;
};

static std::set<const llvm::Value *>
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
      }

      continue;
    }

    if (const auto *GlobalVar =
            std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
      GroundTruthEntries.insert(llvm::cast<llvm::Value>(*GlobalVar));
      continue;
    }

    llvm::report_fatal_error("Unknown variant type.\n");
  }

  return GroundTruthEntries;
};

} // namespace psr

#endif
