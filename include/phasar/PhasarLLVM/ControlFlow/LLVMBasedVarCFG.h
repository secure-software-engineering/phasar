/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARCFG_H_

#include "phasar/ControlFlow/VarCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/VarStaticRenaming.h"

#include "llvm/ADT/StringMap.h"

#include "z3++.h"

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llvm {
class Function;
class Instruction;
class BranchInst;
class BinaryOperator;
class LoadInst;
class CmpInst;
class Constant;
class Value;
class GlobalVariable;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;
class LLVMBasedICFG;

template <> class VarCFGImpl<LLVMBasedICFG, z3::expr> {
  friend VarCFG<LLVMBasedICFG, z3::expr>;
  friend class VariabilityCFGTest;

public:
  explicit VarCFGImpl(
      const LLVMBasedICFG &CFG,
      const stringstringmap_t *StaticBackwardRenaming = nullptr);

  explicit VarCFGImpl(
      const LLVMProjectIRDB &IRDB, const LLVMBasedCFG &CFG,
      const stringstringmap_t *StaticBackwardRenaming = nullptr);

  auto &getContext() noexcept { return CTX; }

private:
  [[nodiscard]] std::vector<std::pair<const llvm::Instruction *, z3::expr>>
  getSuccsOfWithPPConstraintsImpl(const llvm::Instruction *Stmt) const;

  [[nodiscard]] bool isPPBranchTargetImpl(const llvm::Instruction *Stmt,
                                          const llvm::Instruction *Succ) const;

  [[nodiscard]] z3::expr
  getPPConstraintOrTrueImpl(const llvm::Instruction *Stmt,
                            const llvm::Instruction *Succ) const;

  [[nodiscard]] z3::expr getTrueConstraintImpl() const;

  [[nodiscard]] const LLVMBasedCFG &getCFG() const { return CFG; }

  // --- private functions:

  std::optional<z3::expr>
  getConditionIfIsPPVariable(const llvm::GlobalVariable *G) const;

  bool isPPBranchNode(const llvm::BranchInst *BI) const;

  bool isPPBranchNode(const llvm::BranchInst *BI, z3::expr &Cond) const;

  // --- data members

  LLVMBasedCFG CFG{}; // Better to keep it by value -- it only stores a bool
  mutable z3::context CTX;
  llvm::StringMap<z3::expr> AvailablePPConditions;
  const stringstringmap_t *staticBackwardRenaming = nullptr;
};

// namespace detail {
// class LLVMBasedVarCFGImpl
//     : public VarCFG<LLVMBasedVarCFGImpl, const llvm::Instruction *,
//                     const llvm::Function *, z3::expr> {
//   friend VarCFG;

// public:
//   LLVMBasedVarCFGImpl(const LLVMProjectIRDB *IRDB,
//                       const stringstringmap_t *StaticBackwardRenaming =
//                       nullptr, bool IgnoreDbgInstructions = true);

//   z3::context &getContext() const;

//   [[nodiscard]] std::string
//   getDemangledFunctionName(const llvm::Function *Fun) const;

//   bool isPPBranchNode(const llvm::BranchInst *BI) const;

//   bool isPPBranchNode(const llvm::BranchInst *BI, z3::expr &Cond) const;

// private:
//   std::vector<std::pair<const llvm::Instruction *, z3::expr>>
//   getSuccsOfWithPPConstraintsImpl(const llvm::Instruction *Stmt) const;

//   bool isPPBranchTargetImpl(const llvm::Instruction *Stmt,
//                             const llvm::Instruction *Succ) const;

//   z3::expr getPPConstraintOrTrueImpl(const llvm::Instruction *Stmt,
//                                      const llvm::Instruction *Succ) const;

//   z3::expr getTrueConstraintImpl() const;

//   const LLVMBasedCFG &getCFG() const { return CFG; }

//   // ---

//   // z3::expr inferCondition(const llvm::CmpInst *Cmp) const;

//   // z3::expr createExpression(const llvm::Value *V) const;

//   // z3::expr createBinOp(const llvm::BinaryOperator *OP) const;

//   // z3::expr createVariableOrGlobal(const llvm::LoadInst *LI) const;

//   // z3::expr createConstant(const llvm::Constant *C) const;

//   // z3::expr compareBoolAndInt(z3::expr XBool, z3::expr XInt,
//   //                            bool ForEquality) const;

//   // bool isPPVariable(const llvm::GlobalVariable *G, std::string &Name)
//   const;

//   // don't pass by reference, as we need to take ownership of the Name
//   // z3::expr createBoolConst(std::string Name) const;
//   // don't pass by reference, as we need to take ownership of the Name
//   // z3::expr createIntConst(std::string Name) const;

//   std::optional<z3::expr>
//   getConditionIfIsPPVariable(const llvm::GlobalVariable *G) const;

//   z3::expr deserializePPCondition(llvm::StringRef Cond) const;

//   // --- data members

//   // TODO: check if those variables need to be mutable, i.e. the z3
//   // related member functions need to be const.
//   mutable z3::context CTX;
//   llvm::StringMap<z3::expr> AvailablePPConditions;
//   const stringstringmap_t *staticBackwardRenaming = nullptr;
//   LLVMBasedCFG CFG{};
//   // mutable std::unordered_map<std::string, z3::expr> PPVariables;
// };

// } // namespace detail

// class LLVMBasedVarCFG : public detail::LLVMBasedVarCFGImpl,
//                         public LLVMBasedCFG {
// public:
//   LLVMBasedVarCFG(const LLVMProjectIRDB *IRDB,
//                   const stringstringmap_t *StaticBackwardRenaming = nullptr,
//                   bool IgnoreDbgInstructions = true)
//       : detail::LLVMBasedVarCFGImpl(IRDB, StaticBackwardRenaming,
//                                     IgnoreDbgInstructions),
//         LLVMBasedCFG(IgnoreDbgInstructions) {}
// };

} // namespace psr

#endif
