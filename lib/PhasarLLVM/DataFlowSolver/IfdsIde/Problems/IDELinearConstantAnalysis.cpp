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
#include <utility>

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/StrongUpdateStore.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include <utility>

using namespace std;
using namespace psr;

namespace psr {
// Initialize debug counter for edge functions
unsigned IDELinearConstantAnalysis::CurrGenConstant_Id = 0;
unsigned IDELinearConstantAnalysis::CurrLCAID_Id = 0;
unsigned IDELinearConstantAnalysis::CurrBinary_Id = 0;

const IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::TOP =
    numeric_limits<IDELinearConstantAnalysis::l_t>::min();

const IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::BOTTOM =
    numeric_limits<IDELinearConstantAnalysis::l_t>::max();

IDELinearConstantAnalysis::IDELinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

IDELinearConstantAnalysis::~IDELinearConstantAnalysis() {
  IDELinearConstantAnalysis::CurrGenConstant_Id = 0;
  IDELinearConstantAnalysis::CurrLCAID_Id = 0;
  IDELinearConstantAnalysis::CurrBinary_Id = 0;
}

// Start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getNormalFlowFunction(
    IDELinearConstantAnalysis::n_t Curr, IDELinearConstantAnalysis::n_t Succ) {
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (Alloca->getAllocatedType()->isIntegerTy()) {
      return make_shared<Gen<IDELinearConstantAnalysis::d_t>>(Alloca,
                                                              getZeroValue());
    }
  }
  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e. kills previous fact (= pointer operand).
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    IDELinearConstantAnalysis::d_t PointerOp = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      return make_shared<StrongUpdateStore<IDELinearConstantAnalysis::d_t>>(
          Store, [this](IDELinearConstantAnalysis::d_t Source) {
            return Source == getZeroValue();
          });
    }
    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      return make_shared<StrongUpdateStore<IDELinearConstantAnalysis::d_t>>(
          Store, [Store](IDELinearConstantAnalysis::d_t Source) {
            return Source == Store->getValueOperand();
          });
    }
  }
  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    // only consider i32 load
    if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
      return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
          Load, [Load](IDELinearConstantAnalysis::d_t Source) {
            return Source == Load->getPointerOperand();
          });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(Curr)) {
    auto lop = Curr->getOperand(0);
    auto rop = Curr->getOperand(1);
    return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
        Curr, [this, lop, rop](IDELinearConstantAnalysis::d_t Source) {
          return (lop == Source && llvm::isa<llvm::ConstantInt>(rop)) ||
                 (rop == Source && llvm::isa<llvm::ConstantInt>(lop)) ||
                 (Source == getZeroValue() &&
                  !llvm::isa<llvm::ConstantInt>(lop) &&
                  !llvm::isa<llvm::ConstantInt>(rop));
        });
  }
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallFlowFunction(
    IDELinearConstantAnalysis::n_t CallStmt,
    IDELinearConstantAnalysis::f_t DestFun) {
  // Map the actual parameters into the formal parameters
  if (llvm::isa<llvm::CallInst>(CallStmt) ||
      llvm::isa<llvm::InvokeInst>(CallStmt)) {
    struct LCAFF : FlowFunction<const llvm::Value *> {
      vector<const llvm::Value *> Actuals;
      vector<const llvm::Value *> Formals;
      const llvm::Function *DestFun;
      LCAFF(llvm::ImmutableCallSite CallSite,
            IDELinearConstantAnalysis::f_t DestFun)
          : DestFun(DestFun) {
        // Set up the actual parameters
        for (unsigned idx = 0; idx < CallSite.getNumArgOperands(); ++idx) {
          Actuals.push_back(CallSite.getArgOperand(idx));
        }
        // Set up the formal parameters
        for (unsigned idx = 0; idx < DestFun->arg_size(); ++idx) {
          Formals.push_back(getNthFunctionArgument(DestFun, idx));
        }
      }
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t Source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        for (unsigned idx = 0; idx < Actuals.size(); ++idx) {
          if (Source == Actuals[idx]) {
            // Check for C-style varargs: idx >= destFun->arg_size()
            if (idx >= DestFun->arg_size() && !DestFun->isDeclaration()) {
              // Over-approximate by trying to add the
              //   alloca [1 x %struct.__va_list_tag], align 16
              // to the results
              // find the allocated %struct.__va_list_tag and generate it
              for (auto &BB : *DestFun) {
                for (auto &I : BB) {
                  if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                    if (Alloc->getAllocatedType()->isArrayTy() &&
                        Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                        Alloc->getAllocatedType()
                            ->getArrayElementType()
                            ->isStructTy() &&
                        Alloc->getAllocatedType()
                                ->getArrayElementType()
                                ->getStructName() == "struct.__va_list_tag") {
                      res.insert(Alloc);
                    }
                  }
                }
              }
            } else {
              // Ordinary case: Just perform mapping
              res.insert(Formals[idx]); // corresponding formal
            }
          }
          // Special case: Check if function is called with integer literals as
          // parameter (in case of varargs ignore)
          if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
              idx < DestFun->arg_size() &&
              llvm::isa<llvm::ConstantInt>(Actuals[idx])) {
            res.insert(Formals[idx]); // corresponding formal
          }
        }
        if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
            llvm::isa<llvm::GlobalVariable>(Source)) {
          res.insert(Source);
        }
        return res;
      }
    };
    return make_shared<LCAFF>(llvm::ImmutableCallSite(CallStmt), DestFun);
  }
  // Pass everything else as identity
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getRetFlowFunction(
    IDELinearConstantAnalysis::n_t CallSite,
    IDELinearConstantAnalysis::f_t CalleeFun,
    IDELinearConstantAnalysis::n_t ExitStmt,
    IDELinearConstantAnalysis::n_t RetSite) {
  // Handle the case: %x = call i32 ...
  if (CallSite->getType()->isIntegerTy()) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt);
    auto ReturnValue = Return->getReturnValue();
    struct LCAFF : FlowFunction<IDELinearConstantAnalysis::d_t> {
      IDELinearConstantAnalysis::n_t CallSite;
      IDELinearConstantAnalysis::d_t ReturnValue;
      LCAFF(IDELinearConstantAnalysis::n_t CS,
            IDELinearConstantAnalysis::d_t RetVal)
          : CallSite(CS), ReturnValue(RetVal) {}
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t Source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        // Collect return value fact
        if (Source == ReturnValue) {
          res.insert(CallSite);
        }
        // Return value is integer literal
        if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
            llvm::isa<llvm::ConstantInt>(ReturnValue)) {
          res.insert(CallSite);
        }
        if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
            llvm::isa<llvm::GlobalVariable>(Source)) {
          res.insert(Source);
        }
        return res;
      }
    };
    return make_shared<LCAFF>(CallSite, ReturnValue);
  }
  // All other facts except GlobalVariables are killed at this point
  return make_shared<KillIf<IDELinearConstantAnalysis::d_t>>(
      [](IDELinearConstantAnalysis::d_t Source) {
        return !llvm::isa<llvm::GlobalVariable>(Source);
      });
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallToRetFlowFunction(
    IDELinearConstantAnalysis::n_t CallSite,
    IDELinearConstantAnalysis::n_t RetSite, set<f_t> Callees) {
  for (auto callee : Callees) {
    if (!ICF->getStartPointsOf(callee).empty()) {
      return make_shared<KillIf<IDELinearConstantAnalysis::d_t>>(
          [this](IDELinearConstantAnalysis::d_t Source) {
            return !isZeroValue(Source) &&
                   llvm::isa<llvm::GlobalVariable>(Source);
          });
    } else {
      return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
    }
  }
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getSummaryFlowFunction(
    IDELinearConstantAnalysis::n_t CallStmt,
    IDELinearConstantAnalysis::f_t DestFun) {
  return nullptr;
}

map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::initialSeeds() {
  // Check commandline arguments, e.g. argc, and generate all integer
  // typed arguments.
  map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
      SeedMap;
  // Collect global variables of integer type
  for (auto &EntryPoint : EntryPoints) {
    set<IDELinearConstantAnalysis::d_t> Globals;
    for (const auto &G :
         IRDB->getModuleDefiningFunction(EntryPoint)->globals()) {
      if (auto GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
        if (GV->hasInitializer() &&
            llvm::isa<llvm::ConstantInt>(GV->getInitializer())) {
          Globals.insert(GV);
        }
      }
    }
    Globals.insert(getZeroValue());
    if (!Globals.empty()) {
      SeedMap.insert(
          make_pair(&ICF->getFunction(EntryPoint)->front().front(), Globals));
    }
    // Collect commandline arguments of integer type
    if (EntryPoint == "main") {
      set<IDELinearConstantAnalysis::d_t> CmdArgs;
      for (auto &Arg : ICF->getFunction(EntryPoint)->args()) {
        if (Arg.getType()->isIntegerTy()) {
          CmdArgs.insert(&Arg);
        }
      }
      CmdArgs.insert(getZeroValue());
      SeedMap.insert(
          make_pair(&ICF->getFunction(EntryPoint)->front().front(), CmdArgs));
    } else {
      SeedMap.insert(
          make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                    set<IDELinearConstantAnalysis::d_t>({getZeroValue()})));
    }
  }
  return SeedMap;
}

IDELinearConstantAnalysis::d_t
IDELinearConstantAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantAnalysis::isZeroValue(
    IDELinearConstantAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

// In addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(
    IDELinearConstantAnalysis::n_t Curr,
    IDELinearConstantAnalysis::d_t CurrNode,
    IDELinearConstantAnalysis::n_t Succ,
    IDELinearConstantAnalysis::d_t SuccNode) {
  auto &lg = lg::get();
  // Initialize global variables at entry point
  if (!isZeroValue(CurrNode) && ICF->isStartPoint(Curr) &&
      isEntryPoint(ICF->getFunctionOf(Curr)->getName().str()) &&
      llvm::isa<llvm::GlobalVariable>(CurrNode) && CurrNode == SuccNode) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Case: Intialize global variable at entry point.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    auto GV = llvm::dyn_cast<llvm::GlobalVariable>(CurrNode);
    auto CI = llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer());
    auto IntConst = CI->getSExtValue();
    return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
  }

  // All_Bottom for zero value
  if ((isZeroValue(CurrNode) && isZeroValue(SuccNode)) ||
      (llvm::isa<llvm::AllocaInst>(Curr) && isZeroValue(CurrNode))) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Zero value.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
        IDELinearConstantAnalysis::BOTTOM);
  }

  // Check store instruction
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
    if (pointerOperand == SuccNode) {
      // Case I: Storing a constant integer.
      if (isZeroValue(CurrNode) && llvm::isa<llvm::ConstantInt>(valueOperand)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing constant integer.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand);
        auto IntConst = CI->getSExtValue();
        return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
      }
      // Case II: Storing an integer typed value.
      if (CurrNode != SuccNode && valueOperand->getType()->isIntegerTy()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing an integer typed value.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        return make_shared<IDELinearConstantAnalysis::LCAIdentity>();
      }
    }
  }

  // Check load instruction
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    if (Load == SuccNode) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Case: Loading an integer typed value.");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return make_shared<IDELinearConstantAnalysis::LCAIdentity>();
    }
  }

  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (Curr == SuccNode && llvm::isa<llvm::BinaryOperator>(Curr)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Binary operation.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    unsigned OP = Curr->getOpcode();
    auto lop = Curr->getOperand(0);
    auto rop = Curr->getOperand(1);
    // For non linear constant computation we propagate bottom
    if (CurrNode == ZeroValue && !llvm::isa<llvm::ConstantInt>(lop) &&
        !llvm::isa<llvm::ConstantInt>(rop)) {
      return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
          IDELinearConstantAnalysis::BOTTOM);
    } else {
      return make_shared<IDELinearConstantAnalysis::BinOp>(OP, lop, rop,
                                                           CurrNode);
    }
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Edge identity.");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getCallEdgeFunction(
    IDELinearConstantAnalysis::n_t CallStmt,
    IDELinearConstantAnalysis::d_t SrcNode,
    IDELinearConstantAnalysis::f_t DestinationFunction,
    IDELinearConstantAnalysis::d_t DestNode) {
  // Case: Passing constant integer as parameter
  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    if (auto A = llvm::dyn_cast<llvm::Argument>(DestNode)) {
      llvm::ImmutableCallSite CS(CallStmt);
      auto actual = CS.getArgOperand(getFunctionArgumentNr(A));
      if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(actual)) {
        auto IntConst = CI->getSExtValue();
        return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
      }
    }
  }
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getReturnEdgeFunction(
    IDELinearConstantAnalysis::n_t CallSite,
    IDELinearConstantAnalysis::f_t CalleeFunction,
    IDELinearConstantAnalysis::n_t ExitStmt,
    IDELinearConstantAnalysis::d_t ExitNode,
    IDELinearConstantAnalysis::n_t ReSite,
    IDELinearConstantAnalysis::d_t RetNode) {
  // Case: Returning constant integer
  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt);
    auto ReturnValue = Return->getReturnValue();
    if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(ReturnValue)) {
      auto IntConst = CI->getSExtValue();
      return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
    }
  }
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getCallToRetEdgeFunction(
    IDELinearConstantAnalysis::n_t CallSite,
    IDELinearConstantAnalysis::d_t CallNode,
    IDELinearConstantAnalysis::n_t RetSite,
    IDELinearConstantAnalysis::d_t RetSiteNode,
    set<IDELinearConstantAnalysis::f_t> Callees) {
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getSummaryEdgeFunction(
    IDELinearConstantAnalysis::n_t CallStmt,
    IDELinearConstantAnalysis::d_t CallNode,
    IDELinearConstantAnalysis::n_t RetSite,
    IDELinearConstantAnalysis::d_t RetSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::topElement() {
  return TOP;
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::bottomElement() {
  return BOTTOM;
}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::join(IDELinearConstantAnalysis::l_t Lhs,
                                IDELinearConstantAnalysis::l_t Rhs) {
  if (Lhs == TOP && Rhs != BOTTOM) {
    return Rhs;
  } else if (Rhs == TOP && Lhs != BOTTOM) {
    return Lhs;
  } else if (Rhs == Lhs) {
    return Rhs;
  } else {
    return BOTTOM;
  }
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::allTopFunction() {
  return make_shared<AllTop<IDELinearConstantAnalysis::l_t>>(TOP);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAEdgeFunctionComposer::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> SecondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LCAID = dynamic_cast<LCAIdentity *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  return F->composeWith(G->composeWith(SecondFunction));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

IDELinearConstantAnalysis::GenConstant::GenConstant(
    IDELinearConstantAnalysis::l_t IntConst)
    : GenConstant_Id(++IDELinearConstantAnalysis::CurrGenConstant_Id),
      IntConst(IntConst) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::GenConstant::computeTarget(
    IDELinearConstantAnalysis::l_t Source) {
  return IntConst;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> SecondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<LCAIdentity *>(SecondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDELinearConstantAnalysis::LCAEdgeFunctionComposer>(
      this->shared_from_this(), SecondFunction);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::GenConstant::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> Other) const {
  if (auto *GC =
          dynamic_cast<IDELinearConstantAnalysis::GenConstant *>(Other.get())) {
    return (GC->IntConst == this->IntConst);
  }
  return this == Other.get();
}

void IDELinearConstantAnalysis::GenConstant::print(ostream &OS,
                                                   bool IsForDebug) const {
  OS << IntConst << " (EF:" << GenConstant_Id << ')';
}

IDELinearConstantAnalysis::LCAIdentity::LCAIdentity()
    : LCAID_Id(++IDELinearConstantAnalysis::CurrLCAID_Id) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::LCAIdentity::computeTarget(
    IDELinearConstantAnalysis::l_t Source) {
  return Source;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> SecondFunction) {
  return SecondFunction;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::LCAIdentity::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> Other) const {
  return this == Other.get();
}

void IDELinearConstantAnalysis::LCAIdentity::print(ostream &OS,
                                                   bool IsForDebug) const {
  OS << "Id (EF:" << LCAID_Id << ')';
}

IDELinearConstantAnalysis::BinOp::BinOp(const unsigned Op,
                                        IDELinearConstantAnalysis::d_t Lop,
                                        IDELinearConstantAnalysis::d_t Rop,
                                        IDELinearConstantAnalysis::d_t CurrNode)
    : EdgeFunctionID(++IDELinearConstantAnalysis::CurrBinary_Id), Op(Op),
      lop(Lop), rop(Rop), currNode(CurrNode) {}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::BinOp::computeTarget(
    IDELinearConstantAnalysis::l_t Source) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Left Op   : " << llvmIRToString(lop));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Right Op  : " << llvmIRToString(rop));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Curr Node : " << llvmIRToString(currNode));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');

  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(currNode) &&
      llvm::isa<llvm::ConstantInt>(lop) && llvm::isa<llvm::ConstantInt>(rop)) {
    auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
    auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
    return IDELinearConstantAnalysis::executeBinOperation(
        Op, lic->getSExtValue(), ric->getSExtValue());
  } else if (Source == BOTTOM) {
    return BOTTOM;
  } else if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
    auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
    return IDELinearConstantAnalysis::executeBinOperation(Op, Source,
                                                          ric->getSExtValue());
  } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
    auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
    return IDELinearConstantAnalysis::executeBinOperation(
        Op, lic->getSExtValue(), Source);
  }
  throw runtime_error("Only linear constant propagation can be specified!");
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> SecondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<IDELinearConstantAnalysis::LCAIdentity *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDELinearConstantAnalysis::LCAEdgeFunctionComposer>(
      this->shared_from_this(), SecondFunction);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::BinOp::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> Other) const {
  if (auto *BOP =
          dynamic_cast<IDELinearConstantAnalysis::BinOp *>(Other.get())) {
    return BOP->Op == this->Op && BOP->lop == this->lop &&
           BOP->rop == this->rop;
  }
  return this == Other.get();
}

void IDELinearConstantAnalysis::BinOp::print(ostream &OS,
                                             bool IsForDebug) const {
  if (auto LIC = llvm::dyn_cast<llvm::ConstantInt>(lop)) {
    OS << LIC->getSExtValue();
  } else {
    OS << "ID:" << getMetaDataID(lop);
  }
  OS << ' ' << opToChar(Op) << ' ';
  if (auto RIC = llvm::dyn_cast<llvm::ConstantInt>(rop)) {
    OS << RIC->getSExtValue();
  } else {
    OS << "ID:" << getMetaDataID(rop);
  }
}

char IDELinearConstantAnalysis::opToChar(const unsigned Op) {
  char opAsChar;
  switch (Op) {
  case llvm::Instruction::Add:
    opAsChar = '+';
    break;
  case llvm::Instruction::Sub:
    opAsChar = '-';
    break;
  case llvm::Instruction::Mul:
    opAsChar = '*';
    break;
  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    opAsChar = '/';
    break;
  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    opAsChar = '%';
    break;
  default:
    opAsChar = ' ';
    break;
  }
  return opAsChar;
}

bool IDELinearConstantAnalysis::isEntryPoint(std::string FunctionName) const {
  return std::find(EntryPoints.begin(), EntryPoints.end(), FunctionName) !=
         EntryPoints.end();
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::executeBinOperation(
    const unsigned Op, IDELinearConstantAnalysis::l_t Lop,
    IDELinearConstantAnalysis::l_t Rop) {
  // default initialize with BOTTOM (all information)
  IDELinearConstantAnalysis::l_t res = BOTTOM;
  switch (Op) {
  case llvm::Instruction::Add:
    res = Lop + Rop;
    break;

  case llvm::Instruction::Sub:
    res = Lop - Rop;
    break;

  case llvm::Instruction::Mul:
    res = Lop * Rop;
    break;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    res = Lop / Rop;
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    res = Lop % Rop;
    break;

  default:
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Operation not supported by "
                                              "IDELinearConstantAnalysis::"
                                              "executeBinOperation()");
    break;
  }
  return res;
}

void IDELinearConstantAnalysis::printNode(
    ostream &OS, IDELinearConstantAnalysis::n_t N) const {
  OS << llvmIRToShortString(N);
}

void IDELinearConstantAnalysis::printDataFlowFact(
    ostream &OS, IDELinearConstantAnalysis::d_t D) const {
  OS << llvmIRToShortString(D);
}

void IDELinearConstantAnalysis::printFunction(
    ostream &OS, IDELinearConstantAnalysis::f_t M) const {
  OS << M->getName().str();
}

void IDELinearConstantAnalysis::printEdgeFact(
    ostream &OS, IDELinearConstantAnalysis::l_t L) const {
  if (L == BOTTOM) {
    OS << "Bottom";
  } else if (L == TOP) {
    OS << "Top";
  } else {
    OS << std::to_string(L);
  }
}

void IDELinearConstantAnalysis::emitTextReport(
    const SolverResults<IDELinearConstantAnalysis::n_t,
                        IDELinearConstantAnalysis::d_t,
                        IDELinearConstantAnalysis::l_t> &SR,
    std::ostream &OS) {
  OS << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    OS << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (auto f : ICF->getAllFunctions()) {
      std::string fName = getFunctionNameFromIR(f);
      OS << "\nFunction: " << fName << "\n----------"
         << std::string(fName.size(), '-') << '\n';
      for (auto stmt : ICF->getAllInstructionsOf(f)) {
        auto results = SR.resultsAt(stmt, true);
        stripBottomResults(results);
        if (!results.empty()) {
          OS << "At IR statement: " << NtoString(stmt) << '\n';
          for (auto res : results) {
            if (res.second != IDELinearConstantAnalysis::BOTTOM) {
              OS << "   Fact: " << DtoString(res.first)
                 << "\n  Value: " << LtoString(res.second) << '\n';
            }
          }
          OS << '\n';
        }
      }
      OS << '\n';
    }
  } else {
    auto lcaResults = getLCAResults(SR);
    for (auto entry : lcaResults) {
      OS << "\nFunction: " << entry.first
         << "\n==========" << std::string(entry.first.size(), '=') << '\n';
      for (auto fResult : entry.second) {
        fResult.second.print(OS);
        OS << "--------------------------------------\n\n";
      }
      OS << '\n';
    }
  }
}

void IDELinearConstantAnalysis::stripBottomResults(
    std::unordered_map<IDELinearConstantAnalysis::d_t,
                       IDELinearConstantAnalysis::l_t> &Res) {
  for (auto it = Res.begin(); it != Res.end();) {
    if (it->second == IDELinearConstantAnalysis::BOTTOM) {
      it = Res.erase(it);
    } else {
      ++it;
    }
  }
}

IDELinearConstantAnalysis::lca_results_t
IDELinearConstantAnalysis::getLCAResults(
    SolverResults<IDELinearConstantAnalysis::n_t,
                  IDELinearConstantAnalysis::d_t,
                  IDELinearConstantAnalysis::l_t>
        SR) {
  std::map<std::string, std::map<unsigned, LCAResult>> aggResults;
  std::cout << "\n==== Computing LCA Results ====\n";
  for (auto f : ICF->getAllFunctions()) {
    std::string fName = getFunctionNameFromIR(f);
    std::cout << "\n-- Function: " << fName << " --\n";
    std::map<unsigned, LCAResult> fResults;
    std::set<std::string> allocatedVars;
    for (auto stmt : ICF->getAllInstructionsOf(f)) {
      unsigned lnr = getLineFromIR(stmt);
      std::cout << "\nIR : " << NtoString(stmt) << "\nLNR: " << lnr << '\n';
      // We skip statements with no source code mapping
      if (lnr == 0) {
        std::cout << "Skipping this stmt!\n";
        continue;
      }
      LCAResult *lcaRes = &fResults[lnr];
      // Check if it is a new result
      if (lcaRes->src_code.empty()) {
        std::string sourceCode = getSrcCodeFromIR(stmt);
        // Skip results for line containing only closed braces which is the
        // case for functions with void return value
        if (sourceCode == "}") {
          fResults.erase(lnr);
          continue;
        }
        lcaRes->src_code = sourceCode;
        lcaRes->line_nr = lnr;
      }
      lcaRes->ir_trace.push_back(stmt);
      if (stmt->isTerminator() && !ICF->isExitStmt(stmt)) {
        std::cout << "Delete result since stmt is Terminator or Exit!\n";
        fResults.erase(lnr);
      } else {
        // check results of succ(stmt)
        std::unordered_map<d_t, l_t> results;
        if (ICF->isExitStmt(stmt)) {
          results = SR.resultsAt(stmt, true);
        } else {
          // It's not a terminator inst, hence it has only a single successor
          auto succ = ICF->getSuccsOf(stmt)[0];
          std::cout << "Succ stmt: " << NtoString(succ) << '\n';
          results = SR.resultsAt(succ, true);
        }
        stripBottomResults(results);
        std::set<std::string> validVarsAtStmt;
        for (auto res : results) {
          auto varName = getVarNameFromIR(res.first);
          std::cout << "  D: " << DtoString(res.first)
                    << " | V: " << LtoString(res.second)
                    << " | Var: " << varName << '\n';
          if (!varName.empty()) {
            // Only store/overwrite values of variables from allocas or globals
            // unless there is no value stored for a variable
            if (llvm::isa<llvm::AllocaInst>(res.first) ||
                llvm::isa<llvm::GlobalVariable>(res.first)) {
              // lcaRes->variableToValue.find(varName) ==
              // lcaRes->variableToValue.end()) {
              validVarsAtStmt.insert(varName);
              allocatedVars.insert(varName);
              lcaRes->variableToValue[varName] = res.second;
            } else if (allocatedVars.find(varName) == allocatedVars.end()) {
              validVarsAtStmt.insert(varName);
              lcaRes->variableToValue[varName] = res.second;
            }
          }
        }
        // remove no longer valid variables at current IR stmt
        for (auto it = lcaRes->variableToValue.begin();
             it != lcaRes->variableToValue.end();) {
          if (validVarsAtStmt.find(it->first) == validVarsAtStmt.end()) {
            std::cout << "Erase var: " << it->first << '\n';
            it = lcaRes->variableToValue.erase(it);
          } else {
            ++it;
          }
        }
      }
    }
    // delete entries with no result
    for (auto it = fResults.begin(); it != fResults.end();) {
      if (it->second.variableToValue.empty()) {
        it = fResults.erase(it);
      } else {
        ++it;
      }
    }
    aggResults[fName] = fResults;
  }
  return aggResults;
}

void IDELinearConstantAnalysis::LCAResult::print(std::ostream &OS) {
  OS << "Line " << line_nr << ": " << src_code << '\n';
  OS << "Var(s): ";
  for (auto it = variableToValue.begin(); it != variableToValue.end(); ++it) {
    if (it != variableToValue.begin()) {
      OS << ", ";
    }
    OS << it->first << " = " << it->second;
  }
  OS << "\nCorresponding IR Instructions:\n";
  for (auto ir : ir_trace) {
    OS << "  " << llvmIRToString(ir) << '\n';
  }
}

} // namespace psr
