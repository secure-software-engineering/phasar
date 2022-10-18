/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
[[nodiscard]] std::string
LLVMBasedICFG::exportICFGAsDot(bool WithSourceCodeInfo) const {
  std::string Ret;
  /// Just an arbitrary reserve number for now
  const size_t ApproxNumCharsPerInst = WithSourceCodeInfo ? 200 : 80;
  Ret.reserve(IRDB->getNumInstructions() * ApproxNumCharsPerInst);
  llvm::raw_string_ostream OS(Ret);

  OS << "digraph ICFG{\n";
  scope_exit CloseBrace = [&OS]() { OS << "}\n"; };

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto writeSCI = [WithSourceCodeInfo](llvm::raw_ostream &OS,
                                       const llvm::Instruction *Inst) {
    if (WithSourceCodeInfo) {
      auto SCI = getSrcCodeInfoFromIR(Inst);

      OS << "File: ";
      OS.write_escaped(SCI.SourceCodeFilename);
      OS << "\nFunction: ";
      OS.write_escaped(SCI.SourceCodeFunctionName);
      OS << "\nIR: ";
      OS.write_escaped(llvmIRToStableString(Inst));

      if (SCI.Line) {
        OS << "\nLine: " << SCI.Line << "\nColumn: " << SCI.Column;
      }
    } else {
      OS.write_escaped(llvmIRToStableString(Inst));
    }
  };

  auto IgnoreDbgInstructions = this->IgnoreDbgInstructions;

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto createInterEdges = [&OS, this, IgnoreDbgInstructions](
                              const llvm::Instruction *CS, intptr_t To,
                              llvm::StringRef Label) {
    bool HasDecl = false;
    const auto &Callees = getCalleesOfCallAt(CS);

    for (const auto *Callee : Callees) {
      if (Callee->isDeclaration()) {
        HasDecl = true;
        continue;
      }

      // Call Edge
      const auto *BB = &Callee->front();
      assert(BB && !BB->empty());
      const auto *InterTo = &BB->front();

      if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(InterTo)) {
        InterTo = InterTo->getNextNonDebugInstruction(false);
      }
      // createEdge(From, InterTo);
      OS << intptr_t(CS) << "->" << intptr_t(InterTo) << ";\n";

      // Return Edges
      for (const auto *ExitInst : getAllExitPoints(Callee)) {
        /// TODO: Be return/resume aware!
        OS << intptr_t(ExitInst) << "->" << To << Label << ";\n";
      }
    }

    if (HasDecl || Callees.empty()) {
      OS << intptr_t(CS) << "->" << To << Label << ";\n";
    }
  };

  for (const auto *Fun : getAllVertexFunctions()) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(Inst)) {
        continue;
      }

      OS << intptr_t(&Inst) << "[label=\"";
      writeSCI(OS, &Inst);
      OS << "\"];\n";

      if (llvm::isa<llvm::UnreachableInst>(Inst)) {
        continue;
      }

      auto Successors = getSuccsOf(&Inst);

      if (Successors.size() == 2) {
        if (llvm::isa<llvm::InvokeInst>(Inst)) {
          createInterEdges(&Inst, intptr_t(Successors[0]),
                           "[label=\"normal\"]");
          createInterEdges(&Inst, intptr_t(Successors[1]),
                           "[label=\"unwind\"]");
        } else {
          OS << intptr_t(&Inst) << "->" << intptr_t(Successors[0])
             << "[label=\"true\"];\n";
          OS << intptr_t(&Inst) << "->" << intptr_t(Successors[1])
             << "[label=\"false\"];\n";
        }
        continue;
      }

      for (const auto *Successor : Successors) {
        if (llvm::isa<llvm::CallBase>(Inst)) {
          createInterEdges(&Inst, intptr_t(Successor), "");
        } else {
          OS << intptr_t(&Inst) << "->" << intptr_t(Successor) << ";\n";
        }
      }
    }
  }

  OS.flush();
  return Ret;
}

struct SourceCodeInfoWithIR : public SourceCodeInfo {
  std::string IR;
};

static void to_json(nlohmann::json &J, const SourceCodeInfoWithIR &Info) {
  to_json(J, static_cast<const SourceCodeInfo &>(Info));
  J["IR"] = Info.IR;
}

struct GetSCI {
  std::vector<SourceCodeInfoWithIR> SCI;
  llvm::DenseMap<const llvm::Instruction *, SourceCodeInfoWithIR *> Inst2SCI;
  [[maybe_unused]] size_t Capacity{};

  using return_type = SourceCodeInfoWithIR;

  GetSCI(size_t Capacity) {
    SCI.reserve(Capacity);
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    this->Capacity = SCI.capacity();
    Inst2SCI.reserve(Capacity);
  }

  const SourceCodeInfoWithIR &operator()(const llvm::Instruction *Inst) {
    auto &Ret = Inst2SCI[Inst];
    if (Ret) {
      return *Ret;
    }

    auto &Last = SCI.emplace_back(SourceCodeInfoWithIR{
        getSrcCodeInfoFromIR(Inst), llvmIRToStableString(Inst)});
    assert(SCI.capacity() == Capacity &&
           "We must not resize the vector, as otherwise the references are "
           "unstable");

    Ret = &Last;
    return Last;
  }

  static SourceCodeInfoWithIR
  getFirstNonEmpty(llvm::BasicBlock::const_iterator &It,
                   llvm::BasicBlock::const_iterator End) {
    assert(It != End);

    const auto *Inst = &*It;
    auto Ret = getSrcCodeInfoFromIR(Inst);

    // Assume, we aren't skipping relevant calls here

    while ((Ret.empty() || It->isDebugOrPseudoInst()) && ++It != End) {
      Inst = &*It;
      Ret = getSrcCodeInfoFromIR(Inst);
    }

    return {Ret, llvmIRToStableString(Inst)};
  }

  static SourceCodeInfoWithIR getFirstNonEmpty(const llvm::BasicBlock *BB) {
    auto It = BB->begin();
    return getFirstNonEmpty(It, BB->end());
  }
};

struct GetIR {
  std::vector<std::string> IR;
  llvm::DenseMap<const llvm::Instruction *, std::string *> Inst2SCI;
  [[maybe_unused]] size_t Capacity{};

  using return_type = std::string;

  GetIR(size_t Capacity) {
    IR.reserve(Capacity);
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    this->Capacity = IR.capacity();
    Inst2SCI.reserve(Capacity);
  }

  const std::string &operator()(const llvm::Instruction *Inst) {
    assert(Inst);
    auto &Ret = Inst2SCI[Inst];
    if (Ret) {
      return *Ret;
    }

    // llvm::errs() << "> push back IR: " << llvmIRToStableString(Inst) << '\n';

    auto &Last = IR.emplace_back(llvmIRToStableString(Inst));
    assert(IR.capacity() == Capacity &&
           "We must not resize the vector, as otherwise the references are "
           "unstable");

    Ret = &Last;
    return Last;
  }

  static std::string getFirstNonEmpty(llvm::BasicBlock::const_iterator &It,
                                      llvm::BasicBlock::const_iterator End) {
    assert(It != End);

    if (It->isDebugOrPseudoInst()) {
      return llvmIRToStableString(It->getNextNonDebugInstruction());
    }

    return llvmIRToStableString(&*It);
  }

  static std::string getFirstNonEmpty(const llvm::BasicBlock *BB) {
    auto It = BB->begin();
    return getFirstNonEmpty(It, BB->end());
  }
};

template <typename GetSCIFn, typename EdgeCallBack>
static void exportICFGAsSourceCodeImpl(const LLVMBasedICFG &ICF,
                                       GetSCIFn getSCI, // NOLINT
                                       EdgeCallBack &&CreateEdge,
                                       bool IgnoreDbgInstructions) {

  // NOLINTNEXTLINE(readability-identifier-naming)
  // auto isRetVoid = [](const llvm::Instruction *Inst) noexcept {
  //   const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(Inst);
  //   return Ret && !Ret->getReturnValue();
  // };

  using RetTy = typename GetSCIFn::return_type;

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto getLastNonEmpty =
      [/*isRetVoid,*/ &getSCI](const llvm::Instruction *Inst) -> const RetTy & {
    // if (!isRetVoid(Inst) || !Inst->getPrevNode()) {
    //   return getSCI(Inst);
    // }
    // if (const auto *Prev = Inst->getPrevNode()) {
    //   return getSCI(Prev);
    // }

    return getSCI(Inst);
  };

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto createInterEdges = [&ICF, &CreateEdge, &getLastNonEmpty](
                              const llvm::Instruction *CS, const RetTy &From,
                              const RetTy &To) {
    bool NeedCTREdge = false;
    const auto &Callees = ICF.getCalleesOfCallAt(CS);
    for (const auto *Callee : Callees) {
      if (Callee->isDeclaration()) {
        NeedCTREdge = true;
        continue;
      }
      // Call Edge
      RetTy InterTo = GetSCIFn::getFirstNonEmpty(&Callee->front());
      std::invoke(CreateEdge, From, InterTo);

      // Return Edges
      for (const auto *ExitInst : getAllExitPoints(Callee)) {
        std::invoke(CreateEdge, getLastNonEmpty(ExitInst), To);
      }
    }

    if (NeedCTREdge || Callees.empty()) {
      std::invoke(CreateEdge, From, To);
    }
  };

  for (const auto *Fun : ICF.getAllVertexFunctions()) {
    for (const auto &I : llvm::instructions(Fun)) {
      if (llvm::isa<llvm::UnreachableInst>(&I)) {
        continue;
      }
      if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(&I)) {
        continue;
      }

      auto Successors = ICF.getSuccsOf(&I);
      const auto &FromSCI = getSCI(&I);

      for (const auto *Successor : Successors) {
        const RetTy &ToSCI = getSCI(Successor);

        if (llvm::isa<llvm::CallBase>(&I)) {
          createInterEdges(&I, FromSCI, ToSCI);
        } else {
          std::invoke(CreateEdge, FromSCI, ToSCI);
        }
      }
    }
  }
}

[[nodiscard]] nlohmann::json
LLVMBasedICFG::exportICFGAsJson(bool WithSourceCodeInfo) const {
  nlohmann::json J;

  if (WithSourceCodeInfo) {
    exportICFGAsSourceCodeImpl(
        *this, GetSCI(IRDB->getNumInstructions()),
        [&J](const SourceCodeInfoWithIR &From, const SourceCodeInfoWithIR &To) {
          J.push_back({{"from", From}, {"to", To}});
        },
        IgnoreDbgInstructions);
  } else {
    exportICFGAsSourceCodeImpl(
        *this, GetIR(IRDB->getNumInstructions()),
        [&J](const std::string &From, const std::string &To) {
          J.push_back({{"from", From}, {"to", To}});
        },
        IgnoreDbgInstructions);
  }
  return J;
}
} // namespace psr
