#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "SourceMapping.h"

#include <cstdint>
#include <functional>
#include <set>
#include <utility>

namespace psr {

struct SrcCodeLocationEntry {
  SrcCodeLocationEntry(uint32_t Line, uint32_t Column)
      : Line(Line), Column(Column) {}
  SrcCodeLocationEntry(
      uint32_t Line, uint32_t Column,
      std::function<bool(const llvm::Instruction *Inst)> LambdaFunc)
      : Line(Line), Column(Column), UseLambdaFunc(true),
        LambdaFunc(std::move(LambdaFunc)) {}
  uint32_t Line = 0;
  uint32_t Column = 0;

  [[nodiscard]] bool shouldUseLambdaFunc() const { return UseLambdaFunc; }
  [[nodiscard]] std::function<bool(const llvm::Instruction *Inst)>
  getLambdaFunc() const {
    return LambdaFunc;
  }

  bool operator==(const SrcCodeLocationEntry &Other) const {
    return Line == Other.Line && Column == Other.Column;
  }
  bool operator<(const SrcCodeLocationEntry &Other) const {
    return std::tie(Line, Column) < std::tie(Other.Line, Other.Column);
  }

private:
  bool UseLambdaFunc = false;
  std::function<bool(const llvm::Instruction *Inst)> LambdaFunc;
};

static std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
getGroundTruthInsts(
    const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
        &GroundTruth,
    const llvm::Function *Func) {
  std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
      GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    const auto &FirstEntry = std::get<0>(Entry);
    const auto &SecondEntry = std::get<1>(Entry);
    const llvm::Instruction *CurrInst = nullptr;
    const llvm::Value *CurrVal = nullptr;

    if (FirstEntry.shouldUseLambdaFunc()) {
      CurrInst = unittest::getInstAtOrNull(
          Func, FirstEntry.Line, FirstEntry.Column, FirstEntry.getLambdaFunc());
    } else {
      CurrInst =
          unittest::getInstAtOrNull(Func, FirstEntry.Line, FirstEntry.Column);
    }

    if (SecondEntry.shouldUseLambdaFunc()) {
      CurrVal =
          unittest::getInstAtOrNull(Func, SecondEntry.Line, SecondEntry.Column,
                                    SecondEntry.getLambdaFunc());
    } else {
      CurrVal =
          unittest::getInstAtOrNull(Func, SecondEntry.Line, SecondEntry.Column);
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
getGroundTruthInsts(const std::set<SrcCodeLocationEntry> &GroundTruth,
                    const llvm::Function *Func) {
  std::set<const llvm::Instruction *> GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    const llvm::Instruction *CurrInst = nullptr;
    if (Entry.shouldUseLambdaFunc()) {
      CurrInst = unittest::getInstAtOrNull(Func, Entry.Line, Entry.Column,
                                           Entry.getLambdaFunc());
    } else {
      CurrInst = unittest::getInstAtOrNull(Func, Entry.Line, Entry.Column);
    }

    if (CurrInst) {
      GroundTruthEntries.insert(CurrInst);
    }
  }

  return GroundTruthEntries;
};

} // namespace psr

#endif
