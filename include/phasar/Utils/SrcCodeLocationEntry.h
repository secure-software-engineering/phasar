#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
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

    if (std::holds_alternative<const llvm::GlobalVariable *>(
            FirstEntry.Context)) {
      CurrInst = llvm::dyn_cast<llvm::Instruction>(
          std::get<const llvm::Function *>(FirstEntry.Context));
    } else if (FirstEntry.LambdaFunc) {
      CurrInst = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(FirstEntry.Context), FirstEntry.Line,
          FirstEntry.Column, FirstEntry.LambdaFunc);
    } else {
      CurrInst = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(FirstEntry.Context), FirstEntry.Line,
          FirstEntry.Column);
    }

    if (std::holds_alternative<const llvm::GlobalVariable *>(
            FirstEntry.Context)) {
      CurrVal = llvm::dyn_cast<llvm::Value>(
          std::get<const llvm::Function *>(SecondEntry.Context));
    } else if (SecondEntry.LambdaFunc) {
      CurrVal = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(SecondEntry.Context),
          SecondEntry.Line, SecondEntry.Column, SecondEntry.LambdaFunc);
    } else {
      CurrVal = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(SecondEntry.Context),
          SecondEntry.Line, SecondEntry.Column);
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
    const llvm::Instruction *CurrInst = nullptr;
    if (std::holds_alternative<const llvm::GlobalVariable *>(Entry.Context)) {
      CurrInst = llvm::dyn_cast<llvm::Instruction>(
          std::get<const llvm::GlobalVariable *>(Entry.Context));
    } else if (Entry.LambdaFunc) {
      CurrInst = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(Entry.Context), Entry.Line,
          Entry.Column, Entry.LambdaFunc);
    } else {
      CurrInst = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(Entry.Context), Entry.Line,
          Entry.Column);
    }

    if (CurrInst) {
      GroundTruthEntries.insert(CurrInst);
    }
  }

  return GroundTruthEntries;
};

} // namespace psr

#endif
