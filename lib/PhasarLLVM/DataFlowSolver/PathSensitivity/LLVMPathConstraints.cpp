#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

namespace psr {
LLVMPathConstraints::LLVMPathConstraints(z3::context *Z3Ctx,
                                         bool IgnoreDebugInstructions)
    : Z3Ctx(Z3Ctx), IgnoreDebugInstructions(IgnoreDebugInstructions) {
  if (!Z3Ctx) {
    this->Z3Ctx = std::make_unique<z3::context>();
  }
  Z3_set_ast_print_mode(getContext(),
                        Z3_ast_print_mode::Z3_PRINT_SMTLIB2_COMPLIANT);
}

auto LLVMPathConstraints::internalGetConstraintAndVariablesFromEdge(
    const llvm::Instruction *From, const llvm::Instruction *To)
    -> std::optional<ConstraintAndVariables> {
  LLVMBasedCFG CF(IgnoreDebugInstructions);
  const auto *BI = llvm::dyn_cast<llvm::BranchInst>(From);
  if (!BI || !BI->isConditional()) {
    return std::nullopt;
  }

  if (IgnoreDebugInstructions) {
    while (const auto *Prev = To->getPrevNonDebugInstruction(false)) {
      To = Prev;
    }
  } else {
    while (const auto *Prev = To->getPrevNode()) {
      To = Prev;
    }
  }

  if (CF.isBranchTarget(From, To)) {
    return handleCondBrInst(BI, To);
  }

  return std::nullopt;
}

std::optional<z3::expr>
LLVMPathConstraints::getConstraintFromEdge(const llvm::Instruction *From,
                                           const llvm::Instruction *To) {
  if (auto CV = internalGetConstraintAndVariablesFromEdge(From, To)) {
    return CV->Constraint;
  }

  return std::nullopt;
}

auto LLVMPathConstraints::getConstraintAndVariablesFromEdge(
    const llvm::Instruction *From, const llvm::Instruction *To)
    -> std::optional<ConstraintAndVariables> {
  auto CV = internalGetConstraintAndVariablesFromEdge(From, To);
  if (CV) {
    /// Deduplicate the Variables vector
    std::sort(CV->Variables.begin(), CV->Variables.end());
    CV->Variables.erase(std::unique(CV->Variables.begin(), CV->Variables.end()),
                        CV->Variables.end());
  }

  return CV;
}

// void LLVMPathConstraints::getConstraintsInPath(
//     llvm::ArrayRef<const llvm::Instruction *> Path,
//     llvm::SmallVectorImpl<z3::expr> &Dest) {
//   LLVMBasedCFG CF(IgnoreDebugInstructions);
//   for (size_t Idx = 0; Idx < Path.size(); ++Idx) {
//     const llvm::Instruction *I = Path[Idx];
//     // handle non-loop flows and collect branch conditions
//     const auto *BI = llvm::dyn_cast<llvm::BranchInst>(I);
//     if (!BI || !BI->isConditional()) {
//       continue;
//     }

//     // llvm::errs() << "Conditional branch on path: " << *BI << '\n';
//     if (Idx + 1 == Path.size()) {
//       // llvm::errs() << "> skip: last\n";
//       continue;
//     }

//     if (CF.isBranchTarget(I, Path[Idx + 1])) {
//       // llvm::errs() << "> add to solver\n";
//       Dest.push_back(handleCondBrInst(BI, Path[Idx + 1]).Constraint);
//     }
//     // else {
//     //   llvm::errs() << "> skip: no target: " << *Path[Idx + 1] << '\n';
//     // }
//   }
// }

auto LLVMPathConstraints::handlePhiInstruction(const llvm::PHINode *Phi)
    -> ConstraintAndVariables {
  auto Search = Z3Expr.find(Phi);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  const auto *Ty = Phi->getType();
  auto TyConstraints = [&]() {
    std::string Name = "PHI";
    if (const auto *IntTy = llvm::dyn_cast<llvm::IntegerType>(Ty)) {
      auto NumBits = IntTy->getBitWidth();
      // case of bool
      if (NumBits == 1) {
        return Z3Ctx->bool_const(Name.c_str());
      }
      // other integers
      return Z3Ctx->int_const(Name.c_str());
    }
    // case of float types
    if (Ty->isFloatTy() || Ty->isDoubleTy()) {
      return Z3Ctx->real_const(Name.c_str());
    }
    if (Ty->isPointerTy()) {
      /// Treat pointers as integers...
      return Z3Ctx->bv_const(Name.c_str(), 64);
    }
    llvm::report_fatal_error("unhandled type of PHI node!");
  }();
  ConstraintAndVariables CAV{Z3Ctx->bool_val(true), {}};
  for (const auto &Incoming : Phi->incoming_values()) {
    if (const auto *ConstInt = llvm::dyn_cast<llvm::ConstantInt>(Incoming)) {
      if (ConstInt->isOne()) {
        CAV.Constraint = CAV.Constraint || Z3Ctx->bool_val(true);
      } else if (ConstInt->isZero()) {
        CAV.Constraint = CAV.Constraint || Z3Ctx->bool_val(false);
      } else {
        CAV.Constraint = CAV.Constraint || TyConstraints;
      }
    } else if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Incoming)) {
      auto Ret = getLoadInstAsZ3(Load);
      CAV.Constraint = CAV.Constraint || Ret.Constraint;
      CAV.Variables.append(Ret.Variables.begin(), Ret.Variables.end());
    } else if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Incoming)) {
      auto Ret = getFunctionCallAsZ3(Call);
      CAV.Constraint = CAV.Constraint || Ret.Constraint;
      CAV.Variables.append(Ret.Variables.begin(), Ret.Variables.end());
    } else if (const auto *Cmp = llvm::dyn_cast<llvm::CmpInst>(Incoming)) {
      auto Ret = handleCmpInst(Cmp);
      CAV.Constraint = CAV.Constraint || Ret.Constraint;
      CAV.Variables.append(Ret.Variables.begin(), Ret.Variables.end());
    } else {
      llvm::outs() << "unhanled phi value: " << *Incoming << '\n';
      llvm::outs().flush();
      llvm::report_fatal_error("unhanled incoming value in PHI node!");
    }
  }
  return Z3Expr.try_emplace(Phi, CAV).first->second;
}

auto LLVMPathConstraints::handleCondBrInst(const llvm::BranchInst *Br,
                                           const llvm::Instruction *Succ)
    -> ConstraintAndVariables {
  // llvm::outs() << "handleCondBrInst\n";
  auto Search = Z3Expr.find(Br);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  const auto *Cond = Br->getCondition();
  // yes, the true label is indeed operand 2
  const auto *IfTrue = llvm::dyn_cast<llvm::BasicBlock>(Br->getOperand(2));
  const auto *IfFalse = llvm::dyn_cast<llvm::BasicBlock>(Br->getOperand(1));

  auto GetFirstInst = [IgnoreDebugInstructions{IgnoreDebugInstructions}](
                          const llvm::BasicBlock *BB) {
    const auto *Ret = &BB->front();
    if (IgnoreDebugInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(Ret)) {
      Ret = Ret->getNextNonDebugInstruction(false);
    }
    return Ret;
  };

  if (const auto *Cmp = llvm::dyn_cast<llvm::CmpInst>(Cond)) {
    if (IfTrue && GetFirstInst(IfTrue) == Succ) {
      return handleCmpInst(Cmp);
    }
    if (IfFalse && GetFirstInst(IfFalse) == Succ) {
      auto Ret = handleCmpInst(Cmp);
      Ret.Constraint = !Ret.Constraint;
      return Ret;
    }
  }
  if (const auto *BinOp = llvm::dyn_cast<llvm::BinaryOperator>(Cond)) {
    if (IfTrue && GetFirstInst(IfTrue) == Succ) {
      return handleBinaryOperator(BinOp);
    }
    if (IfFalse && GetFirstInst(IfFalse) == Succ) {
      auto Ret = handleBinaryOperator(BinOp);
      Ret.Constraint = !Ret.Constraint;
      return Ret;
    }
  }
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Cond)) {
    if (IfTrue && GetFirstInst(IfTrue) == Succ) {
      return getFunctionCallAsZ3(CallSite);
    }
    if (IfFalse && GetFirstInst(IfFalse) == Succ) {
      auto Ret = getFunctionCallAsZ3(CallSite);
      Ret.Constraint = !Ret.Constraint;
      return Ret;
    }
  }
  // Dirty HACK
  if (const auto *Trunc = llvm::dyn_cast<llvm::TruncInst>(Cond)) {
    if (Trunc->getDestTy()->isIntegerTy(1)) {
      if (const auto *FromLoad =
              llvm::dyn_cast<llvm::LoadInst>(Trunc->getOperand(0))) {
        auto Ret = getLoadInstAsZ3(FromLoad);
        Ret.Constraint = (Ret.Constraint % 2 == 1);
        return Ret;
      }
    }
  }
  if (const auto *Phi = llvm::dyn_cast<llvm::PHINode>(Cond)) {
    return handlePhiInstruction(Phi);
  }
  llvm::report_fatal_error(
      llvm::StringRef("unhandled conditional branch instruction: '") +
      llvmIRToString(Br) + "'!\n");
}

auto LLVMPathConstraints::handleCmpInst(const llvm::CmpInst *Cmp)
    -> ConstraintAndVariables {
  // llvm::outs() << "handleCmpInst\n";
  auto Search = Z3Expr.find(Cmp);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  const auto *Lhs = Cmp->getOperand(0);
  const auto *Rhs = Cmp->getOperand(1);

  auto HandleOperand = [this](const llvm::Value *Op) {
    if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Op)) {
      Op = Cast->getOperand(0);
    }

    if (const auto *ConstData = llvm::dyn_cast<llvm::ConstantData>(Op)) {
      return getLiteralAsZ3(ConstData);
    }
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Op)) {
      if (const auto *Alloca =
              llvm::dyn_cast<llvm::AllocaInst>(Load->getPointerOperand())) {
        return getAllocaInstAsZ3(Alloca);
      }
      return getLoadInstAsZ3(Load);
    }
    if (const auto *BinOpLhs = llvm::dyn_cast<llvm::BinaryOperator>(Op)) {
      return handleBinaryOperator(BinOpLhs);
    }
    if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Op)) {
      return getFunctionCallAsZ3(CallSite);
    }
    if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Op)) {
      return getGEPInstAsZ3(Gep);
    }
    llvm::report_fatal_error("unhandled operand: " +
                             llvm::Twine(llvmIRToString(Op)));
  };

  auto LhsZ3Expr = HandleOperand(Lhs);
  auto RhsZ3Expr = HandleOperand(Rhs);
  LhsZ3Expr.Variables.append(RhsZ3Expr.Variables);

  switch (Cmp->getPredicate()) {
    // Opcode            U L G E    Intuitive operation
  case llvm::CmpInst::Predicate::FCMP_FALSE: ///< 0 0 0 0    Always false
    ///< (always folded)
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_OEQ: ///< 0 0 0 1    True if ordered and
    ///< equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_OGT: ///< 0 0 1 0    True if ordered and
    ///< greater than
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_OGE: ///< 0 0 1 1    True if ordered and
    ///< greater than or equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_OLT: ///< 0 1 0 0    True if ordered and
    ///< less than
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_OLE: ///< 0 1 0 1    True if ordered and
    ///< less than or equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_ONE: ///< 0 1 1 0    True if ordered and
    ///< operands are unequal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_ORD: ///< 0 1 1 1    True if ordered (no
    ///< nans)
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_UNO: ///< 1 0 0 0    True if unordered:
    ///< isnan(X) | isnan(Y)
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_UEQ: ///< 1 0 0 1    True if unordered or
    ///< equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_UGT: ///< 1 0 1 0    True if unordered or
    ///< greater than
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_UGE: ///< 1 0 1 1    True if unordered,
    ///< greater than, or equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_ULT: ///< 1 1 0 0    True if unordered or
    ///< less than
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_ULE: ///< 1 1 0 1    True if unordered,
    ///< less than, or equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_UNE: ///< 1 1 1 0    True if unordered or
    ///< not equal
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::FCMP_TRUE: ///< 1 1 1 1    Always true (always
    ///< folded)
    llvm::report_fatal_error("unhandled predicate!");
    break;
  case llvm::CmpInst::Predicate::ICMP_EQ: ///< equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint == RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_NE: ///< not equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint != RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_UGT: ///< unsigned greater than
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint > RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_UGE: ///< unsigned greater or equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint >= RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_ULT: ///< unsigned less than
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint < RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_ULE: ///< unsigned less or equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint <= RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_SGT: ///< signed greater than
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint > RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_SGE: ///< signed greater or equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint >= RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_SLT: ///< signed less than
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint < RhsZ3Expr.Constraint;
    break;
  case llvm::CmpInst::Predicate::ICMP_SLE: ///< signed less or equal
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint < RhsZ3Expr.Constraint;
    break;
  default:
    llvm::report_fatal_error("unhandled predicate!");
    break;
  }

  return LhsZ3Expr;
}

auto LLVMPathConstraints::handleBinaryOperator(
    const llvm::BinaryOperator *BinOp) -> ConstraintAndVariables {
  auto Search = Z3Expr.find(BinOp);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  const auto *Lhs = BinOp->getOperand(0);
  const auto *Rhs = BinOp->getOperand(1);

  auto HandleOperand = [this](const llvm::Value *Op) {
    if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Op)) {
      Op = Cast->getOperand(0);
    }

    if (const auto *ConstData = llvm::dyn_cast<llvm::ConstantData>(Op)) {
      return getLiteralAsZ3(ConstData);
    }
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Op)) {
      if (const auto *Alloca =
              llvm::dyn_cast<llvm::AllocaInst>(Load->getPointerOperand())) {
        return getAllocaInstAsZ3(Alloca);
      }
      return getLoadInstAsZ3(Load);
    }
    if (const auto *BinOpLhs = llvm::dyn_cast<llvm::BinaryOperator>(Op)) {
      return handleBinaryOperator(BinOpLhs);
    }
    if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Op)) {
      return getFunctionCallAsZ3(CallSite);
    }
    llvm::report_fatal_error("unhandled operand: " +
                             llvm::Twine(llvmIRToString(Op)));
  };

  auto LhsZ3Expr = HandleOperand(Lhs);
  auto RhsZ3Expr = HandleOperand(Rhs);
  LhsZ3Expr.Variables.append(RhsZ3Expr.Variables);

  /// TODO: Once we have bitvectors, we must distinguish between signed and
  /// unsigned div/rem operations!

  switch (BinOp->getOpcode()) {
  case llvm::Instruction::BinaryOps::Add:
  case llvm::Instruction::BinaryOps::FAdd:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint + RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::Sub:
  case llvm::Instruction::BinaryOps::FSub:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint - RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::Mul:
  case llvm::Instruction::BinaryOps::FMul:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint * RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::UDiv:
  case llvm::Instruction::BinaryOps::SDiv:
  case llvm::Instruction::BinaryOps::FDiv:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint / RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::URem:
  case llvm::Instruction::BinaryOps::SRem:
  case llvm::Instruction::BinaryOps::FRem:
    LhsZ3Expr.Constraint = z3::rem(LhsZ3Expr.Constraint, RhsZ3Expr.Constraint);
    break;
  case llvm::Instruction::BinaryOps::Shl: // Shift left  (logical)
    LhsZ3Expr.Constraint = z3::shl(LhsZ3Expr.Constraint, RhsZ3Expr.Constraint);
    break;
  case llvm::Instruction::BinaryOps::LShr: // Shift right (logical)
    LhsZ3Expr.Constraint = z3::lshr(LhsZ3Expr.Constraint, RhsZ3Expr.Constraint);
    break;
  case llvm::Instruction::BinaryOps::AShr: // Shift right (arithmetic)
    LhsZ3Expr.Constraint = z3::ashr(LhsZ3Expr.Constraint, RhsZ3Expr.Constraint);
    break;
  case llvm::Instruction::BinaryOps::And:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint & RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::Or:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint | RhsZ3Expr.Constraint;
    break;
  case llvm::Instruction::BinaryOps::Xor:
    LhsZ3Expr.Constraint = LhsZ3Expr.Constraint ^ RhsZ3Expr.Constraint;
    break;
  default:
    llvm::report_fatal_error("unhandled binary opcode!");
    break;
  }

  return Z3Expr.try_emplace(BinOp, LhsZ3Expr).first->second;
}

static llvm::DILocalVariable *getDILocalVariable(const llvm::AllocaInst *A) {
  const llvm::Function *Fun = A->getParent()->getParent();
  // Search for llvm.dbg.declare
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      if (const auto *Dbg = llvm::dyn_cast<llvm::DbgDeclareInst>(&I)) {
        // found. is it for an AllocaInst?
        if (auto *DbgAI = llvm::dyn_cast<llvm::AllocaInst>(Dbg->getAddress())) {
          // is it for our AllocaInst?
          if (DbgAI == A) {
            if (llvm::DILocalVariable *VarMD = Dbg->getVariable()) {
              return VarMD;
            }
          }
        }
      }
    }
  }
  return nullptr;
}

static llvm::StringRef getVarName(const llvm::AllocaInst *A) {
  auto *VarMD = getDILocalVariable(A);
  if (VarMD) {
    return VarMD->getName();
  }
  if (A->hasName()) {
    return A->getName();
  }
  return "";
}

auto LLVMPathConstraints::getAllocaInstAsZ3(const llvm::AllocaInst *Alloca)
    -> ConstraintAndVariables {
  auto Search = Z3Expr.find(Alloca);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  std::string Name = getVarName(Alloca).str();
  const auto *Ty = Alloca->getAllocatedType();

  if (const auto *IntTy = llvm::dyn_cast<llvm::IntegerType>(Ty)) {
    auto NumBits = IntTy->getBitWidth();
    // case of bool
    if (NumBits == 1) {
      auto BoolConst = Z3Ctx->bool_const(Name.c_str());
      return Z3Expr
          .try_emplace(Alloca, ConstraintAndVariables{BoolConst, {Alloca}})
          .first->second;
    }
    // other integers
    auto IntConst = Z3Ctx->int_const(Name.c_str());
    return Z3Expr
        .try_emplace(Alloca, ConstraintAndVariables{IntConst, {Alloca}})
        .first->second;
  }
  // case of float types
  if (Ty->isFloatTy() || Ty->isDoubleTy()) {
    auto RealConst = Z3Ctx->real_const(Name.c_str());
    return Z3Expr
        .try_emplace(Alloca, ConstraintAndVariables{RealConst, {Alloca}})
        .first->second;
  }
  if (Ty->isPointerTy()) {
    /// Treat pointers as integers...
    return Z3Expr
        .try_emplace(
            Alloca,
            ConstraintAndVariables{Z3Ctx->bv_const(Name.c_str(), 64), {Alloca}})
        .first->second;
  }
  llvm::report_fatal_error(
      "unsupported type: " + llvm::Twine(llvmTypeToString(Ty)) + " at " +
      llvmIRToString(Alloca) + " in function " +
      Alloca->getFunction()->getName().str());
}

auto LLVMPathConstraints::getLoadInstAsZ3(const llvm::LoadInst *Load)
    -> ConstraintAndVariables {
  auto Search = Z3Expr.find(Load);
  if (Search != Z3Expr.end()) {
    return Search->second;
  }
  if (const auto *Alloca =
          llvm::dyn_cast<llvm::AllocaInst>(Load->getPointerOperand())) {
    return getAllocaInstAsZ3(Alloca);
  }
  // do not follow GEP instructions
  if (const auto *GEP =
          llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand())) {
    return getGEPInstAsZ3(GEP);
  }
  // track down alloca or GEP
  llvm::SmallVector<const llvm::Value *> Worklist = {Load};
  while (!Worklist.empty()) {
    const auto *WorkV = Worklist.pop_back_val();
    // llvm::outs() << "Load-Val: " << *WorkV << '\n';
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(WorkV)) {
      return getAllocaInstAsZ3(Alloca);
    }
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(WorkV)) {
      return getGEPInstAsZ3(GEP);
    }
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(WorkV)) {
      Worklist.append(Inst->op_begin(), Inst->op_end());
    }
  }
  llvm::report_fatal_error("unsupported load!");
}

auto LLVMPathConstraints::getGEPInstAsZ3(const llvm::GetElementPtrInst *GEP)
    -> ConstraintAndVariables {
  std::string Name =
      (GEP->hasName()) ? GEP->getName().str() : "gep" + psr::getMetaDataID(GEP);

  /// TODO: Unify different GEPs of same memory locations
  const auto *Ty = GEP->getResultElementType();
  if (const auto *IntTy = llvm::dyn_cast<llvm::IntegerType>(Ty)) {
    auto NumBits = IntTy->getBitWidth();
    // case of bool
    if (NumBits == 1) {
      auto BoolConst = Z3Ctx->bool_const(Name.c_str());
      return Z3Expr.try_emplace(GEP, ConstraintAndVariables{BoolConst, {GEP}})
          .first->second;
    }
    // other integers
    auto IntConst = Z3Ctx->int_const(Name.c_str());
    // also add value constraints to the solver
    // FIXME
    // if (isSignedInteger(GEP)) {
    //   auto Range = IntConst >= std::numeric_limits<int>::min() &&
    //                IntConst <= std::numeric_limits<int>::max();
    //   Z3Solver.add(Range);
    // } else if (isUnsignedInteger(GEP)) {
    // Z3 only supports plain 'int'
    // Z3Solver.add(Range);
    // }
    return Z3Expr.try_emplace(GEP, ConstraintAndVariables{IntConst, {GEP}})
        .first->second;
  }
  // case of float types
  if (Ty->isFloatTy() || Ty->isDoubleTy()) {
    auto RealConst = Z3Ctx->real_const(Name.c_str());
    return Z3Expr.try_emplace(GEP, ConstraintAndVariables{RealConst, {GEP}})
        .first->second;
  }
  // some other pointer, we can treat those as integers
  if (const auto *PointerTy =
          llvm::dyn_cast<llvm::PointerType>(GEP->getPointerOperandType())) {
    auto PointerConst = Z3Ctx->bv_const(Name.c_str(), 64);
    return Z3Expr.try_emplace(GEP, ConstraintAndVariables{PointerConst, {GEP}})
        .first->second;
  }
  llvm::report_fatal_error("unsupported GEP type!");
}

auto LLVMPathConstraints::getLiteralAsZ3(const llvm::Value *V)
    -> ConstraintAndVariables {
  if (const auto *CI = llvm::dyn_cast<llvm::ConstantInt>(V)) {
    return {Z3Ctx->int_val(CI->getSExtValue()), {}};
  }
  if (const auto *CF = llvm::dyn_cast<llvm::ConstantFP>(V)) {
    return {Z3Ctx->fpa_val(CF->getValue().convertToDouble()), {}};
  }
  if (llvm::isa<llvm::ConstantPointerNull>(V)) {
    return {Z3Ctx->bv_val(0, 64), {}};
  }
  llvm::report_fatal_error("unhandled literal!");
}

auto LLVMPathConstraints::getFunctionCallAsZ3(const llvm::CallBase *CallSite)
    -> ConstraintAndVariables {
  if (const auto *Callee = CallSite->getCalledFunction()) {
    if (Callee->hasName()) {
      auto DemangledName = llvm::demangle(Callee->getName().str());
      const auto *ReturnTy = Callee->getReturnType();
      if (ReturnTy->isFloatingPointTy()) {
        return {Z3Ctx->real_const(DemangledName.c_str()), {Callee}};
      }
      if (ReturnTy->isIntegerTy()) {
        // case of bool
        if (ReturnTy->getIntegerBitWidth() == 1) {
          return {Z3Ctx->bool_const(DemangledName.c_str()), {Callee}};
        }
        return {Z3Ctx->int_const(DemangledName.c_str()), {Callee}};
      }
      if (ReturnTy->isPointerTy()) {
        return {Z3Ctx->bv_const(DemangledName.c_str(), 64), {Callee}};
      }
    }
  }
  llvm::report_fatal_error("unhandled function call!");
}

} // namespace psr