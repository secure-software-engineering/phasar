#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "SourceMapping.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <ostream>
#include <set>
#include <string>
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

  [[nodiscard]] std::string str() const {
    return std::string("GlobalVar { Name: ") + Name.str() + " }";
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
  [[nodiscard]] std::string str() const {
    return std::string("LineCol { Line: ") + std::to_string(Line) +
           "; Col: " + std::to_string(Col) + " }";
  }
};
struct LineColFun {
  uint32_t Line{};
  uint32_t Col{};
  llvm::StringRef InFunction{};

  friend bool operator<(LineColFun LC1, LineColFun LC2) noexcept {
    return std::tie(LC1.InFunction, LC1.Line, LC1.Col) <
           std::tie(LC2.InFunction, LC2.Line, LC2.Col);
  }
  friend bool operator==(LineColFun LC1, LineColFun LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col, LC1.InFunction) ==
           std::tie(LC2.Line, LC2.Col, LC2.InFunction);
  }
  [[nodiscard]] std::string str() const {
    return std::string("LineColFun { Line: ") + std::to_string(Line) +
           "; Col: " + std::to_string(Col) +
           "; InFunction: " + InFunction.str() + " }";
  }
};

struct LineColFunOp {
  uint32_t Line{};
  uint32_t Col{};
  llvm::StringRef InFunction{};
  uint32_t OpCode{};

  friend bool operator<(LineColFunOp LC1, LineColFunOp LC2) noexcept {
    return std::tie(LC1.InFunction, LC1.Line, LC1.Col, LC1.OpCode) <
           std::tie(LC2.InFunction, LC2.Line, LC2.Col, LC2.OpCode);
  }
  friend bool operator==(LineColFunOp LC1, LineColFunOp LC2) noexcept {
    return std::tie(LC1.Line, LC1.Col, LC1.InFunction, LC1.OpCode) ==
           std::tie(LC2.Line, LC2.Col, LC2.InFunction, LC2.OpCode);
  }
  [[nodiscard]] std::string str() const {
    return std::string("LineColFunOp { Line: ") + std::to_string(Line) +
           "; Col: " + std::to_string(Col) +
           "; InFunction: " + InFunction.str() +
           "; OpCode: " + llvm::Instruction::getOpcodeName(OpCode) + " }";
  }
};
struct ArgNo {
  uint32_t Idx{};

  friend bool operator<(ArgNo A1, ArgNo A2) noexcept { return A1.Idx < A2.Idx; }
  friend bool operator==(ArgNo A1, ArgNo A2) noexcept {
    return A1.Idx == A2.Idx;
  }
  [[nodiscard]] std::string str() const {
    return std::string("ArgNo { Idx: ") + std::to_string(Idx) + " }";
  }
};
struct ArgInFun {
  uint32_t Idx;
  llvm::StringRef InFunction{};

  friend bool operator<(ArgInFun A1, ArgInFun A2) noexcept {
    return std::tie(A1.InFunction, A1.Idx) < std::tie(A2.InFunction, A2.Idx);
  }
  friend bool operator==(ArgInFun A1, ArgInFun A2) noexcept {
    return std::tie(A1.Idx, A1.InFunction) == std::tie(A2.Idx, A2.InFunction);
  }
  [[nodiscard]] std::string str() const {
    return std::string("ArgInFun { Idx: ") + std::to_string(Idx) +
           "; InFunction: " + InFunction.str() + " }";
  }
};

struct RetVal {
  llvm::StringRef InFunction;

  friend bool operator<(RetVal R1, RetVal R2) noexcept {
    return R1.InFunction < R2.InFunction;
  }
  friend bool operator==(RetVal R1, RetVal R2) noexcept {
    return R1.InFunction == R2.InFunction;
  }
  [[nodiscard]] std::string str() const {
    return std::string("RetVal { InFunction: ") + InFunction.str() + +" }";
  }
};
struct RetStmt {
  llvm::StringRef InFunction;

  friend bool operator<(RetStmt R1, RetStmt R2) noexcept {
    return R1.InFunction < R2.InFunction;
  }
  friend bool operator==(RetStmt R1, RetStmt R2) noexcept {
    return R1.InFunction == R2.InFunction;
  }
  [[nodiscard]] std::string str() const {
    return std::string("RetStmt { InFunction: ") + InFunction.str() + +" }";
  }
};

struct TestingSrcLocation
    : public std::variant<LineCol, LineColFun, LineColFunOp, GlobalVar, ArgNo,
                          ArgInFun, RetVal, RetStmt> {
  using VarT = std::variant<LineCol, LineColFun, LineColFunOp, GlobalVar, ArgNo,
                            ArgInFun, RetVal, RetStmt>;
  using VarT::variant;

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
  [[nodiscard]] std::string str() const {
    return std::visit([](const auto &Val) { return Val.str(); }, *this);
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const TestingSrcLocation &Loc) {
    return OS << Loc.str();
  }
  friend std::ostream &operator<<(std::ostream &OS,
                                  const TestingSrcLocation &Loc) {
    return OS << Loc.str();
  }
};

} // namespace psr

namespace std {
template <> struct hash<psr::LineCol> {
  size_t operator()(psr::LineCol LC) const noexcept {
    return llvm::hash_value(std::make_pair(LC.Line, LC.Col));
  }
};
template <> struct hash<psr::LineColFun> {
  size_t operator()(psr::LineColFun LCF) const noexcept {
    return llvm::hash_combine(
        llvm::hash_value(std::make_pair(LCF.Line, LCF.Col)), LCF.InFunction);
  }
};
template <> struct hash<psr::LineColFunOp> {
  size_t operator()(psr::LineColFunOp LCF) const noexcept {
    return llvm::hash_combine(
        llvm::hash_value(std::make_pair(LCF.Line, LCF.Col)), LCF.InFunction,
        LCF.OpCode);
  }
};
template <> struct hash<psr::GlobalVar> {
  size_t operator()(psr::GlobalVar GV) const noexcept {
    return llvm::hash_value(GV.Name);
  }
};
template <> struct hash<psr::ArgNo> {
  size_t operator()(psr::ArgNo Arg) const noexcept {
    return llvm::hash_value(Arg.Idx);
  }
};
template <> struct hash<psr::ArgInFun> {
  size_t operator()(psr::ArgInFun Arg) const noexcept {
    return llvm::hash_combine(Arg.Idx, Arg.InFunction);
  }
};

template <> struct hash<psr::RetVal> {
  size_t operator()(psr::RetVal Ret) const noexcept {
    return llvm::hash_value(Ret.InFunction);
  }
};

template <> struct hash<psr::RetStmt> {
  size_t operator()(psr::RetStmt Ret) const noexcept {
    return llvm::hash_value(Ret.InFunction);
  }
};

template <> struct hash<psr::TestingSrcLocation> {
  size_t operator()(const psr::TestingSrcLocation &Loc) const noexcept {
    return std::hash<psr::TestingSrcLocation::VarT>{}(Loc);
  }
};
} // namespace std

namespace psr {

[[nodiscard]] inline const llvm::Value *
testingLocInIR(TestingSrcLocation Loc, const LLVMProjectIRDB &IRDB,
               const llvm::Function *InterestingFunction = nullptr) {
  const auto GetFunction = [&IRDB](llvm::StringRef Name) {
    const auto *InFun = IRDB.getFunctionDefinition(Name);
    if (!InFun) {
      llvm::report_fatal_error("Required function '" + Name +
                               "' does not exist in the IR!");
    }
    return InFun;
  };
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
          [&](LineColFun LC) -> llvm ::Value const * {
            const auto *InFun = GetFunction(LC.InFunction);
            return unittest::getInstAtOrNull(InFun, LC.Line, LC.Col);
          },
          [&](LineColFunOp LC) -> llvm ::Value const * {
            const auto *InFun = GetFunction(LC.InFunction);
            return unittest::getInstAtOrNull(
                InFun, LC.Line, LC.Col, [Op = LC.OpCode](const auto *Inst) {
                  return Inst->getOpcode() == Op;
                });
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
          [&](ArgInFun A) -> llvm ::Value const * {
            const auto *InFun = GetFunction(A.InFunction);
            if (InFun->arg_size() <= A.Idx) {
              llvm::report_fatal_error("Argument index " + llvm::Twine(A.Idx) +
                                       " is out of range (" +
                                       llvm::Twine(InFun->arg_size()) + ")!");
            }
            return InFun->getArg(A.Idx);
          },
          [&](RetVal R) -> llvm::Value const * {
            const auto *InFun = GetFunction(R.InFunction);
            for (const auto &BB : llvm::reverse(InFun->getBasicBlockList())) {
              if (const auto *Ret =
                      llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator())) {
                return Ret->getReturnValue();
              }
            }
            llvm::report_fatal_error("No return stmt in function " +
                                     R.InFunction);
          },
          [&](RetStmt R) -> llvm::Value const * {
            const auto *InFun = GetFunction(R.InFunction);
            for (const auto &BB : llvm::reverse(InFun->getBasicBlockList())) {
              if (const auto *Ret =
                      llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator())) {
                return Ret;
              }
            }
            llvm::report_fatal_error("No return stmt in function " +
                                     R.InFunction);
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

template <typename MapTy>
[[nodiscard]] inline auto convertTestingLocationSetMapInIR(
    const MapTy &Locs, const LLVMProjectIRDB &IRDB,
    const llvm::Function *InterestingFunction = nullptr) {
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>> Ret;
  llvm::transform(
      Locs, std::inserter(Ret, Ret.end()), [&](const auto &LocAndSet) {
        const auto &[InstLoc, Set] = LocAndSet;
        const auto *LocVal = llvm::dyn_cast_if_present<llvm::Instruction>(
            testingLocInIR(InstLoc, IRDB, InterestingFunction));
        auto ConvSet =
            convertTestingLocationSetInIR(Set, IRDB, InterestingFunction);
        return std::make_pair(LocVal, std::move(ConvSet));
      });
  return Ret;
}

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

inline const llvm::Instruction *
getGroundTruthInst(const SrcCodeLocationEntry &Entry) {
  if (const auto *GlobalVar =
          std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
    return llvm::dyn_cast_or_null<llvm::Instruction>(*GlobalVar);
  }

  if (const auto *Func = std::get_if<const llvm::Function *>(&Entry.Context)) {
    if (Entry.LambdaFunc) {
      return unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column,
                                       Entry.LambdaFunc);
    }

    return unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column);
  }

  llvm::report_fatal_error("Unknown variant type.\n");
}

inline const llvm::Instruction *
getInstFromEntryOrNull(const SrcCodeLocationEntry &Entry) {
  if (const auto *GlobalVar =
          std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
    return llvm::dyn_cast_or_null<llvm::Instruction>(*GlobalVar);
  }

  if (const auto *Func = std::get_if<const llvm::Function *>(&Entry.Context)) {
    if (Entry.LambdaFunc) {
      return unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column,
                                       Entry.LambdaFunc);
    }
    return unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column);
  }

  llvm::report_fatal_error("Unknown variant type.\n");
}

inline const llvm::Value *
getValueFromEntryOrNull(const SrcCodeLocationEntry &Entry) {
  if (const auto *GlobalVar =
          std::get_if<const llvm::GlobalVariable *>(&Entry.Context)) {
    return llvm::dyn_cast_or_null<llvm::Value>(*GlobalVar);
  }

  if (const auto *Func = std::get_if<const llvm::Function *>(&Entry.Context)) {
    if (Entry.LambdaFunc) {
      if (const auto *Inst = unittest::getInstAtOrNull(
              *Func, Entry.Line, Entry.Column, Entry.LambdaFunc)) {
        return llvm::dyn_cast_or_null<llvm::Value>(Inst);
      }
    }
    if (const auto *Inst =
            unittest::getInstAtOrNull(*Func, Entry.Line, Entry.Column)) {
      return llvm::dyn_cast_or_null<llvm::Value>(Inst);
    }
  }

  llvm::report_fatal_error("Unknown variant type.\n");
}

inline std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
getGroundTruthInsts(
    const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
        &GroundTruth,
    bool PrintData = false) {
  std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
      GroundTruthEntries;

  for (const auto &Entry : GroundTruth) {
    const auto &FirstEntry = std::get<0>(Entry);
    const auto &SecondEntry = std::get<1>(Entry);
    const llvm::Instruction *CurrInst = getInstFromEntryOrNull(FirstEntry);
    const llvm::Value *CurrVal = getValueFromEntryOrNull(SecondEntry);

    if (!CurrInst) {
      if (PrintData) {
        llvm::outs() << "FirstEntry.Line: " << FirstEntry.Line
                     << " - FirstEntry.Column: " << FirstEntry.Column << "\n";
      }
      llvm::report_fatal_error("Couldn't cast first entry to instruction\n");
      continue;
    }

    if (!CurrVal) {
      if (PrintData) {
        llvm::outs() << "SecondEntry.Line: " << SecondEntry.Line
                     << " - SecondEntry.Column: " << SecondEntry.Column << "\n";
      }
      llvm::report_fatal_error("Couldn't cast second entry to value\n");
      continue;
    }

    if (PrintData) {
      llvm::outs() << "FirstEntry.Line: " << FirstEntry.Line
                   << " - FirstEntry.Column: " << FirstEntry.Column << "\n";
      llvm::outs() << "CurrInst: " << CurrInst << "\n";
      llvm::outs() << "llvmIRToString(CurrInst): " << llvmIRToString(CurrInst)
                   << "\n";
      llvm::outs() << "\nSecondEntry.Line: " << SecondEntry.Line
                   << " - SecondEntry.Column: " << SecondEntry.Column << "\n";
      llvm::outs() << "CurrVal: " << CurrVal << "\n";
      llvm::outs() << "llvmIRToString(CurrVal): " << llvmIRToString(CurrVal)
                   << "\n";
    }
    GroundTruthEntries.insert({CurrInst, CurrVal});
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
        llvm::errs() << "*Func: " << **Func << "\n";
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
