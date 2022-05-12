/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

// #include <functional>
#include <limits>
#include <memory>
#include <utility>

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarPass/Options.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

namespace psr {
// Initialize debug counter for edge functions
unsigned IDELinearConstantAnalysis::CurrGenConstantId = 0; // NOLINT
unsigned IDELinearConstantAnalysis::CurrLCAIDId = 0;       // NOLINT
unsigned IDELinearConstantAnalysis::CurrBinaryId = 0;      // NOLINT

const IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::TOP = Top{};

const IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::BOTTOM =
    Bottom{};

IDELinearConstantAnalysis::IDELinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue =
      IDELinearConstantAnalysis::createZeroValue();
}

IDELinearConstantAnalysis::~IDELinearConstantAnalysis() {
  CurrGenConstantId = 0;
  CurrLCAIDId = 0;
  CurrBinaryId = 0;
}

// Start formulating our analysis by specifying the parts required for IFDS

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getNormalFlowFunction(n_t Curr, n_t /*Succ*/) {
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (Alloca->getAllocatedType()->isIntegerTy()) {
      return std::make_shared<Gen<d_t>>(Alloca, getZeroValue());
    }
  }
  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e., kills previous fact (= pointer operand).
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    d_t ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      // return Identity<d_t>::getInstance();
      return std::make_shared<StrongUpdateStore<d_t>>(
          Store, [this](d_t Source) { return Source == getZeroValue(); });
    }
    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      return std::make_shared<StrongUpdateStore<d_t>>(
          Store,
          [Store](d_t Source) { return Source == Store->getValueOperand(); });
    }
  }
  // check load instructions
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    // only consider i32 load
    if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
      return std::make_shared<GenIf<d_t>>(Load, [Load](d_t Source) {
        return Source == Load->getPointerOperand();
      });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(Curr)) {
    auto *Lop = Curr->getOperand(0);
    auto *Rop = Curr->getOperand(1);
    return std::make_shared<GenIf<d_t>>(Curr, [this, Lop, Rop](d_t Source) {
      /// Intentionally include nonlinear operations here for being able to
      /// explicitly set them to BOTTOM in the edge function
      return (Lop == Source) || (Rop == Source) ||
             (isZeroValue(Source) && llvm::isa<llvm::ConstantInt>(Lop) &&
              llvm::isa<llvm::ConstantInt>(Rop));
    });
  }
  return Identity<d_t>::getInstance();
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getCallFlowFunction(n_t CallSite, f_t DestFun) {
  // Map the actual parameters into the formal parameters
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {

    struct LCAFF : FlowFunction<const llvm::Value *> {
      std::vector<const llvm::Value *> Actuals;
      std::vector<const llvm::Value *> Formals;
      const llvm::Function *DestFun;
      LCAFF(const llvm::CallBase *CallSite, f_t DestFun) : DestFun(DestFun) {
        // std::set up the actual parameters
        for (unsigned Idx = 0; Idx < CallSite->getNumArgOperands(); ++Idx) {
          Actuals.push_back(CallSite->getArgOperand(Idx));
        }
        // std::set up the formal parameters
        for (unsigned Idx = 0; Idx < DestFun->arg_size(); ++Idx) {
          Formals.push_back(getNthFunctionArgument(DestFun, Idx));
        }
      }
      std::set<d_t> computeTargets(d_t Source) override {
        // std::cout << "call call-ff with: " << llvmIRToString(Source) << '\n';
        std::set<d_t> Res;
        for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
          if (Source == Actuals[Idx]) {
            // Check for C-style varargs: idx >= destFun->arg_size()
            if (Idx >= DestFun->arg_size() && !DestFun->isDeclaration()) {
              // Over-approximate by trying to add the
              //   alloca [1 x %struct.__va_list_tag], align 16
              // to the results
              // find the allocated %struct.__va_list_tag and generate it
              for (const auto &BB : *DestFun) {
                for (const auto &I : BB) {
                  if (const auto *Alloc =
                          llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                    if (Alloc->getAllocatedType()->isArrayTy() &&
                        Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                        Alloc->getAllocatedType()
                            ->getArrayElementType()
                            ->isStructTy() &&
                        Alloc->getAllocatedType()
                                ->getArrayElementType()
                                ->getStructName() == "struct.__va_list_tag") {
                      Res.insert(Alloc);
                    }
                  }
                }
              }
            } else {
              // Ordinary case: Just perform mapping
              Res.insert(Formals[Idx]); // corresponding formal
            }
          }
          // Special case: Check if function is called with integer literals as
          // parameter (in case of varargs ignore)
          if (LLVMZeroValue::isLLVMZeroValue(Source) &&
              Idx < DestFun->arg_size() &&
              llvm::isa<llvm::ConstantInt>(Actuals[Idx])) {
            Res.insert(Formals[Idx]); // corresponding formal
          }
        }
        if (!LLVMZeroValue::isLLVMZeroValue(Source) &&
            llvm::isa<llvm::GlobalVariable>(Source)) {
          Res.insert(Source);
        }
        return Res;
      }
    };

    if (!DestFun->isDeclaration()) {
      return std::make_shared<LCAFF>(CS, DestFun);
    }
  }
  // Pass everything else as identity
  return Identity<d_t>::getInstance();
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                              n_t ExitInst, n_t /*RetSite*/) {
  // Handle the case: %x = call i32 ...
  if (CallSite->getType()->isIntegerTy()) {
    const auto *Return = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
    auto *ReturnValue = Return ? Return->getReturnValue() : nullptr;

    if (ReturnValue) {
      struct LCAFF : FlowFunction<d_t> {
        n_t CallSite;
        d_t ReturnValue;
        LCAFF(n_t CS, d_t RetVal) : CallSite(CS), ReturnValue(RetVal) {}
        std::set<d_t> computeTargets(d_t Source) override {
          std::set<d_t> Res;
          // Collect return value fact
          if (Source == ReturnValue) {
            Res.insert(CallSite);
          }
          // Return value is integer literal
          if (LLVMZeroValue::isLLVMZeroValue(Source) &&
              llvm::isa<llvm::ConstantInt>(ReturnValue)) {
            Res.insert(CallSite);
          }
          if (!LLVMZeroValue::isLLVMZeroValue(Source) &&
              llvm::isa<llvm::GlobalVariable>(Source)) {
            Res.insert(Source);
          }
          return Res;
        }
      };
      return std::make_shared<LCAFF>(CallSite, ReturnValue);
    }
  }
  // All other facts except GlobalVariables are killed at this point
  return std::make_shared<KillIf<d_t>>(
      [](d_t Source) { return !llvm::isa<llvm::GlobalVariable>(Source); });
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getCallToRetFlowFunction(n_t /*CallSite*/,
                                                    n_t /*RetSite*/,
                                                    std::set<f_t> Callees) {
  for (const auto *Callee : Callees) {
    if (!ICF->getStartPointsOf(Callee).empty()) {
      return std::make_shared<KillIf<d_t>>([this](d_t Source) {
        return !isZeroValue(Source) && llvm::isa<llvm::GlobalVariable>(Source);
      });
    }
  }
  return Identity<d_t>::getInstance();
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getSummaryFlowFunction(n_t /*CallSite*/,
                                                  f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
             IDELinearConstantAnalysis::l_t>
IDELinearConstantAnalysis::initialSeeds() {
  InitialSeeds<n_t, d_t, l_t> Seeds;
  // The analysis' entry points
  std::set<const llvm::Function *> EntryPointFuns;

  // Consider the user-defined entry point(s)
  if (EntryPoints.size() == 1U &&
      EntryPoints.find("__ALL__") != EntryPoints.end()) {
    // Consider all available function definitions as entry points
    for (const auto *Fun : IRDB->getAllFunctions()) {
      if (!Fun->isDeclaration()) {
        EntryPointFuns.insert(Fun);
      }
    }
  } else {
    // Consider the user specified entry points
    for (const auto &EntryPoint : EntryPoints) {
      EntryPointFuns.insert(IRDB->getFunctionDefinition(EntryPoint));
    }
  }

  // std::set initial seeds at the required entry points and generate global
  // integer-typed variables using generalized initial seeds
  for (const auto *EntryPointFun : EntryPointFuns) {
    Seeds.addSeed(&EntryPointFun->front().front(), getZeroValue(),
                  bottomElement());
    // Generate global integer-typed variables using generalized initial seeds
    for (const auto *M : IRDB->getAllModules()) {
      for (const auto &G : M->globals()) {
        if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
          if (GV->hasInitializer()) {
            if (const auto *ConstInt =
                    llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer())) {
              Seeds.addSeed(&EntryPointFun->front().front(), GV,
                            ConstInt->getSExtValue());
            }
          }
        }
      }
    }
  }

  return Seeds;
}

IDELinearConstantAnalysis::d_t
IDELinearConstantAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantAnalysis::isZeroValue(d_t Fact) const {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

// In addition provide specifications for the IDE parts

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                 n_t /*Succ*/, d_t SuccNode) {
  // ALL_BOTTOM for zero value
  if ((isZeroValue(CurrNode) && isZeroValue(SuccNode)) ||
      (llvm::isa<llvm::AllocaInst>(Curr) && isZeroValue(CurrNode))) {
    PHASAR_LOG_LEVEL(DEBUG, "Case: Zero value.");
    PHASAR_LOG_LEVEL(DEBUG, ' ');
    return std::make_shared<AllBottom<l_t>>(BOTTOM);
  }

  // Check store instruction
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    d_t PointerOperand = Store->getPointerOperand();
    d_t ValueOperand = Store->getValueOperand();
    if (PointerOperand == SuccNode) {
      // Case I: Storing a constant integer.
      if (isZeroValue(CurrNode) && llvm::isa<llvm::ConstantInt>(ValueOperand)) {
        PHASAR_LOG_LEVEL(DEBUG, "Case: Storing constant integer.");
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        const auto *CI = llvm::dyn_cast<llvm::ConstantInt>(ValueOperand);
        auto IntConst = CI->getSExtValue();
        return std::make_shared<GenConstant>(IntConst);
      }
      // Case II: Storing an integer typed value.
      if (CurrNode != SuccNode && ValueOperand->getType()->isIntegerTy()) {
        PHASAR_LOG_LEVEL(DEBUG, "Case: Storing an integer typed value.");
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        return std::make_shared<LCAIdentity>();
      }
    }
  }

  // Check load instruction
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    if (Load == SuccNode) {
      PHASAR_LOG_LEVEL(DEBUG, "Case: Loading an integer typed value.");
      PHASAR_LOG_LEVEL(DEBUG, ' ');
      return std::make_shared<LCAIdentity>();
    }
  }

  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (Curr == SuccNode && CurrNode != SuccNode &&
      llvm::isa<llvm::BinaryOperator>(Curr)) {
    PHASAR_LOG_LEVEL(DEBUG, "Case: Binary operation.");
    PHASAR_LOG_LEVEL(DEBUG, ' ');
    unsigned OP = Curr->getOpcode();
    auto *Lop = Curr->getOperand(0);
    auto *Rop = Curr->getOperand(1);
    // For non linear constant computation we propagate bottom
    if ((CurrNode == Lop && !llvm::isa<llvm::ConstantInt>(Rop)) ||
        (CurrNode == Rop && !llvm::isa<llvm::ConstantInt>(Lop))) {
      return std::make_shared<AllBottom<l_t>>(BOTTOM);
    }

    return std::make_shared<BinOp>(OP, Lop, Rop, CurrNode);
  }

  PHASAR_LOG_LEVEL(DEBUG, "Case: Edge identity.");
  PHASAR_LOG_LEVEL(DEBUG, ' ');
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                               f_t /*DestinationFunction*/,
                                               d_t DestNode) {
  // Case: Passing constant integer as parameter
  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    if (const auto *A = llvm::dyn_cast<llvm::Argument>(DestNode)) {
      const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
      const auto *Actual = CS->getArgOperand(getFunctionArgumentNr(A));
      if (const auto *CI = llvm::dyn_cast<llvm::ConstantInt>(Actual)) {
        auto IntConst = CI->getSExtValue();
        return std::make_shared<GenConstant>(IntConst);
      }
    }
  }
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getReturnEdgeFunction(n_t /*CallSite*/,
                                                 f_t /*CalleeFunction*/,
                                                 n_t ExitStmt, d_t ExitNode,
                                                 n_t /*RetSite*/, d_t RetNode) {
  // Case: Returning constant integer
  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    const auto *Return = llvm::cast<llvm::ReturnInst>(ExitStmt);
    auto *ReturnValue = Return->getReturnValue();
    if (auto *CI = llvm::dyn_cast_or_null<llvm::ConstantInt>(ReturnValue)) {
      auto IntConst = CI->getSExtValue();
      return std::make_shared<GenConstant>(IntConst);
    }
  }
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getCallToRetEdgeFunction(n_t /*CallSite*/,
                                                    d_t /*CallNode*/,
                                                    n_t /*RetSite*/,
                                                    d_t /*RetSiteNode*/,
                                                    std::set<f_t> /*Callees*/) {
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getSummaryEdgeFunction(n_t /*CallSite*/,
                                                  d_t /*CallNode*/,
                                                  n_t /*RetSite*/,
                                                  d_t /*RetSiteNode*/) {
  return nullptr;
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::topElement() {
  return TOP;
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::bottomElement() {
  return BOTTOM;
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::join(l_t Lhs,
                                                               l_t Rhs) {
  if (Rhs == Lhs || Lhs == TOP) {
    return Rhs;
  }
  if (Rhs == TOP) {
    return Lhs;
  }
  return BOTTOM;
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::allTopFunction() {
  return std::make_shared<AllTop<l_t>>(TOP);
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAEdgeFunctionComposer::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> SecondFunction) {
  if (dynamic_cast<AllBottom<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<EdgeIdentity<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<LCAIdentity *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  return F->composeWith(G->composeWith(SecondFunction));
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAEdgeFunctionComposer::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllTop<l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<l_t>>(BOTTOM);
}

IDELinearConstantAnalysis::GenConstant::GenConstant(int64_t IntConst)
    : GenConstantId(++CurrGenConstantId), IntConst(IntConst) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::GenConstant::computeTarget(
    IDELinearConstantAnalysis::l_t /*Source*/) {
  return IntConst;
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> SecondFunction) {
  if (dynamic_cast<AllBottom<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<EdgeIdentity<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<LCAIdentity *>(SecondFunction.get())) {
    return this->shared_from_this();
  }

  auto Res = SecondFunction->computeTarget(IntConst);

  if (Res == TOP) {
    return std::make_shared<AllTop<l_t>>(TOP);
  }
  if (Res == BOTTOM) {
    return std::make_shared<AllBottom<l_t>>(BOTTOM);
  }

  return std::make_shared<GenConstant>(std::get<int64_t>(Res));
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllTop<l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<l_t>>(BOTTOM);
}

bool IDELinearConstantAnalysis::GenConstant::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> Other) const {
  if (auto *GC = dynamic_cast<GenConstant *>(Other.get())) {
    return (GC->IntConst == this->IntConst);
  }
  return this == Other.get();
}

void IDELinearConstantAnalysis::GenConstant::print(llvm::raw_ostream &OS,
                                                   bool /*IsForDebug*/) const {
  OS << IntConst << " (EF:" << GenConstantId << ')';
}

IDELinearConstantAnalysis::LCAIdentity::LCAIdentity()
    : LCAIDId(++CurrLCAIDId) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::LCAIdentity::computeTarget(l_t Source) {
  return Source;
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> SecondFunction) {
  return SecondFunction;
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllTop<l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<l_t>>(BOTTOM);
}

bool IDELinearConstantAnalysis::LCAIdentity::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> Other) const {
  return this == Other.get();
}

void IDELinearConstantAnalysis::LCAIdentity::print(llvm::raw_ostream &OS,
                                                   bool /*IsForDebug*/) const {
  OS << "Id (EF:" << LCAIDId << ')';
}

IDELinearConstantAnalysis::BinOp::BinOp(const unsigned Op, d_t Lop, d_t Rop,
                                        d_t CurrNode)
    : EdgeFunctionID(++CurrBinaryId), Op(Op), Lop(Lop), Rop(Rop),
      CurrNode(CurrNode) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::BinOp::computeTarget(l_t Source) {
  PHASAR_LOG_LEVEL(DEBUG, "Left Op   : " << llvmIRToString(Lop));
  PHASAR_LOG_LEVEL(DEBUG, "Right Op  : " << llvmIRToString(Rop));
  PHASAR_LOG_LEVEL(DEBUG, "Curr Node : " << llvmIRToString(CurrNode));
  PHASAR_LOG_LEVEL(DEBUG, ' ');

  if (LLVMZeroValue::isLLVMZeroValue(CurrNode) &&
      llvm::isa<llvm::ConstantInt>(Lop) && llvm::isa<llvm::ConstantInt>(Rop)) {
    const auto *Lic = llvm::cast<llvm::ConstantInt>(Lop);
    const auto *Ric = llvm::cast<llvm::ConstantInt>(Rop);
    return executeBinOperation(Op, Lic->getSExtValue(), Ric->getSExtValue());
  }
  if (Source == BOTTOM) {
    return BOTTOM;
  }
  if (Lop == CurrNode && llvm::isa<llvm::ConstantInt>(Rop)) {
    const auto *Ric = llvm::cast<llvm::ConstantInt>(Rop);
    return executeBinOperation(Op, Source, Ric->getSExtValue());
  }
  if (Rop == CurrNode && llvm::isa<llvm::ConstantInt>(Lop)) {
    const auto *Lic = llvm::cast<llvm::ConstantInt>(Lop);
    return executeBinOperation(Op, Lic->getSExtValue(), Source);
  }

  throw std::runtime_error(
      "Only linear constant propagation can be specified!");
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> SecondFunction) {
  if (dynamic_cast<AllBottom<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<EdgeIdentity<l_t> *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<LCAIdentity *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(SecondFunction.get())) {
    return SecondFunction;
  }
  return std::make_shared<LCAEdgeFunctionComposer>(this->shared_from_this(),
                                                   SecondFunction);
}

std::shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllTop<l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<l_t>>(BOTTOM);
}

bool IDELinearConstantAnalysis::BinOp::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> Other) const {
  if (auto *BOP = dynamic_cast<BinOp *>(Other.get())) {
    return BOP->Op == this->Op && BOP->Lop == this->Lop &&
           BOP->Rop == this->Rop;
  }
  return this == Other.get();
}

void IDELinearConstantAnalysis::BinOp::print(llvm::raw_ostream &OS,
                                             bool /*IsForDebug*/) const {
  if (const auto *LIC = llvm::dyn_cast<llvm::ConstantInt>(Lop)) {
    OS << LIC->getSExtValue();
  } else {
    OS << "ID:" << getMetaDataID(Lop);
  }
  OS << ' ' << opToChar(Op) << ' ';
  if (const auto *RIC = llvm::dyn_cast<llvm::ConstantInt>(Rop)) {
    OS << RIC->getSExtValue();
  } else {
    OS << "ID:" << getMetaDataID(Rop);
  }
}

char IDELinearConstantAnalysis::opToChar(const unsigned Op) {
  switch (Op) {
  case llvm::Instruction::Add:
    return '+';
  case llvm::Instruction::Sub:
    return '-';
  case llvm::Instruction::Mul:
    return '*';
  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    return '/';
  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    return '%';
  case llvm::Instruction::And:
    return '&';
  case llvm::Instruction::Or:
    return '|';
  case llvm::Instruction::Xor:
    return '^';
  default:
    return ' ';
  }
}

bool IDELinearConstantAnalysis::isEntryPoint(
    const std::string &FunctionName) const {
  return EntryPoints.count(FunctionName);
}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::executeBinOperation(const unsigned Op, l_t LVal,
                                               l_t RVal) {

  auto *LopPtr = std::get_if<int64_t>(&LVal);
  auto *RopPtr = std::get_if<int64_t>(&RVal);

  if (!LopPtr || !RopPtr) {
    return BOTTOM;
  }

  auto Lop = *LopPtr;
  auto Rop = *RopPtr;

  // default initialize with BOTTOM (all information)
  int64_t Res;
  switch (Op) {
  case llvm::Instruction::Add:
    if (llvm::AddOverflow(Lop, Rop, Res)) {
      return BOTTOM;
    }
    return Res;

  case llvm::Instruction::Sub:
    if (llvm::SubOverflow(Lop, Rop, Res)) {
      return BOTTOM;
    }
    return Res;

  case llvm::Instruction::Mul:
    if (llvm::MulOverflow(Lop, Rop, Res)) {
      return BOTTOM;
    }
    return Res;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    if (Lop == std::numeric_limits<int64_t>::min() &&
        Rop == -1) { // Would produce and overflow, as the complement of min is
                     // not representable in a signed type.
      return TOP;
    }
    if (Rop == 0) { // Division by zero is UB, so we return Bot
      return BOTTOM;
    }
    return Lop / Rop;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    if (Rop == 0) { // Division by zero is UB, so we return Bot
      return BOTTOM;
    }
    return Lop % Rop;

  case llvm::Instruction::And:
    return Lop & Rop;
  case llvm::Instruction::Or:
    return Lop | Rop;
  case llvm::Instruction::Xor:
    return Lop ^ Rop;
  default:
    PHASAR_LOG_LEVEL(DEBUG, "Operation not supported by "
                            "IDELinearConstantAnalysis::"
                            "executeBinOperation()");
    return BOTTOM;
  }
}

void IDELinearConstantAnalysis::printNode(llvm::raw_ostream &OS,
                                          n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IDELinearConstantAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                                  d_t Fact) const {
  OS << llvmIRToShortString(Fact);
}

void IDELinearConstantAnalysis::printFunction(llvm::raw_ostream &OS,
                                              f_t Func) const {
  OS << Func->getName();
}

void IDELinearConstantAnalysis::printEdgeFact(llvm::raw_ostream &OS,
                                              l_t L) const {
  OS << L;
}

void IDELinearConstantAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, llvm::raw_ostream &OS) {
  OS << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    OS << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (const auto *F : ICF->getAllFunctions()) {
      std::string FName = getFunctionNameFromIR(F);
      OS << "\nFunction: " << FName << "\n----------"
         << std::string(FName.size(), '-') << '\n';
      for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
        auto Results = SR.resultsAt(Stmt, true);
        stripBottomResults(Results);
        if (!Results.empty()) {
          OS << "At IR statement: " << NtoString(Stmt) << '\n';
          for (auto Res : Results) {
            if (Res.second != BOTTOM) {
              OS << "   Fact: " << DtoString(Res.first)
                 << "\n  Value: " << LtoString(Res.second) << '\n';
            }
          }
          OS << '\n';
        }
      }
      OS << '\n';
    }
  } else {
    auto LcaResults = getLCAResults(SR);
    for (const auto &Entry : LcaResults) {
      OS << "\nFunction: " << Entry.first
         << "\n==========" << std::string(Entry.first.size(), '=') << '\n';
      for (auto FResult : Entry.second) {
        FResult.second.print(OS);
        OS << "--------------------------------------\n\n";
      }
      OS << '\n';
    }
  }
}

void IDELinearConstantAnalysis::stripBottomResults(
    std::unordered_map<d_t, l_t> &Res) {
  for (auto It = Res.begin(); It != Res.end();) {
    if (It->second == BOTTOM) {
      It = Res.erase(It);
    } else {
      ++It;
    }
  }
}

IDELinearConstantAnalysis::lca_results_t
IDELinearConstantAnalysis::getLCAResults(SolverResults<n_t, d_t, l_t> SR) {
  std::map<std::string, std::map<unsigned, LCAResult>> AggResults;
  llvm::outs() << "\n==== Computing LCA Results ====\n";
  for (const auto *F : ICF->getAllFunctions()) {
    std::string FName = getFunctionNameFromIR(F);
    llvm::outs() << "\n-- Function: " << FName << " --\n";
    std::map<unsigned, LCAResult> FResults;
    std::set<std::string> AllocatedVars;
    for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
      unsigned Lnr = getLineFromIR(Stmt);
      llvm::outs() << "\nIR : " << NtoString(Stmt) << "\nLNR: " << Lnr << '\n';
      // We skip statements with no source code mapping
      if (Lnr == 0) {
        llvm::outs() << "Skipping this stmt!\n";
        continue;
      }
      LCAResult *LcaRes = &FResults[Lnr];
      // Check if it is a new result
      if (LcaRes->SrcNode.empty()) {
        std::string SourceCode = getSrcCodeFromIR(Stmt);
        // Skip results for line containing only closed braces which is the
        // case for functions with void return value
        if (SourceCode == "}") {
          FResults.erase(Lnr);
          continue;
        }
        LcaRes->SrcNode = SourceCode;
        LcaRes->LineNr = Lnr;
      }
      LcaRes->IRTrace.push_back(Stmt);
      if (Stmt->isTerminator() && !ICF->isExitInst(Stmt)) {
        llvm::outs() << "Delete result since stmt is Terminator or Exit!\n";
        FResults.erase(Lnr);
      } else {
        // check results of succ(stmt)
        std::unordered_map<d_t, l_t> Results;
        if (ICF->isExitInst(Stmt)) {
          Results = SR.resultsAt(Stmt, true);
        } else {
          // It's not a terminator inst, hence it has only a single successor
          const auto *Succ = ICF->getSuccsOf(Stmt)[0];
          llvm::outs() << "Succ stmt: " << NtoString(Succ) << '\n';
          Results = SR.resultsAt(Succ, true);
        }
        stripBottomResults(Results);
        std::set<std::string> ValidVarsAtStmt;
        for (auto Res : Results) {
          auto VarName = getVarNameFromIR(Res.first);
          llvm::outs() << "  D: " << DtoString(Res.first)
                       << " | V: " << LtoString(Res.second)
                       << " | Var: " << VarName << '\n';
          if (!VarName.empty()) {
            // Only store/overwrite values of variables from allocas or globals
            // unless there is no value stored for a variable
            if (llvm::isa<llvm::AllocaInst>(Res.first) ||
                llvm::isa<llvm::GlobalVariable>(Res.first)) {
              // lcaRes->variableToValue.find(varName) ==
              // lcaRes->variableToValue.end()) {
              ValidVarsAtStmt.insert(VarName);
              AllocatedVars.insert(VarName);
              LcaRes->VariableToValue[VarName] = Res.second;
            } else if (AllocatedVars.find(VarName) == AllocatedVars.end()) {
              ValidVarsAtStmt.insert(VarName);
              LcaRes->VariableToValue[VarName] = Res.second;
            }
          }
        }
        // remove no longer valid variables at current IR stmt
        for (auto It = LcaRes->VariableToValue.begin();
             It != LcaRes->VariableToValue.end();) {
          if (ValidVarsAtStmt.find(It->first) == ValidVarsAtStmt.end()) {
            llvm::outs() << "Erase var: " << It->first << '\n';
            It = LcaRes->VariableToValue.erase(It);
          } else {
            ++It;
          }
        }
      }
    }
    // delete entries with no result
    for (auto It = FResults.begin(); It != FResults.end();) {
      if (It->second.VariableToValue.empty()) {
        It = FResults.erase(It);
      } else {
        ++It;
      }
    }
    AggResults[FName] = FResults;
  }
  return AggResults;
}

void IDELinearConstantAnalysis::LCAResult::print(llvm::raw_ostream &OS) {
  OS << this;
}

} // namespace psr
