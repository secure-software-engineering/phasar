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
    IDELinearConstantAnalysis::n_t curr, IDELinearConstantAnalysis::n_t succ) {
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isIntegerTy()) {
      return make_shared<Gen<IDELinearConstantAnalysis::d_t>>(Alloca,
                                                              getZeroValue());
    }
  }
  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e. kills previous fact (= pointer operand).
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t PointerOp = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      return make_shared<StrongUpdateStore<IDELinearConstantAnalysis::d_t>>(
          Store, [this](IDELinearConstantAnalysis::d_t source) {
            return source == getZeroValue();
          });
    }
    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      return make_shared<StrongUpdateStore<IDELinearConstantAnalysis::d_t>>(
          Store, [Store](IDELinearConstantAnalysis::d_t source) {
            return source == Store->getValueOperand();
          });
    }
  }
  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    // only consider i32 load
    if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
      return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
          Load, [Load](IDELinearConstantAnalysis::d_t source) {
            return source == Load->getPointerOperand();
          });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(curr)) {
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
        curr, [this, lop, rop](IDELinearConstantAnalysis::d_t source) {
          return (lop == source && llvm::isa<llvm::ConstantInt>(rop)) ||
                 (rop == source && llvm::isa<llvm::ConstantInt>(lop)) ||
                 (source == getZeroValue() &&
                  !llvm::isa<llvm::ConstantInt>(lop) &&
                  !llvm::isa<llvm::ConstantInt>(rop));
        });
  }
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::f_t destFun) {
  // Map the actual parameters into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    struct LCAFF : FlowFunction<const llvm::Value *> {
      vector<const llvm::Value *> actuals;
      vector<const llvm::Value *> formals;
      const llvm::Function *destFun;
      LCAFF(llvm::ImmutableCallSite callSite,
            IDELinearConstantAnalysis::f_t destFun)
          : destFun(destFun) {
        // Set up the actual parameters
        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
          actuals.push_back(callSite.getArgOperand(idx));
        }
        // Set up the formal parameters
        for (unsigned idx = 0; idx < destFun->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destFun, idx));
        }
      }
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        for (unsigned idx = 0; idx < actuals.size(); ++idx) {
          if (source == actuals[idx]) {
            // Check for C-style varargs: idx >= destFun->arg_size()
            if (idx >= destFun->arg_size() && !destFun->isDeclaration()) {
              // Over-approximate by trying to add the
              //   alloca [1 x %struct.__va_list_tag], align 16
              // to the results
              // find the allocated %struct.__va_list_tag and generate it
              for (auto &BB : *destFun) {
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
              res.insert(formals[idx]); // corresponding formal
            }
          }
          // Special case: Check if function is called with integer literals as
          // parameter (in case of varargs ignore)
          if (LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
              idx < destFun->arg_size() &&
              llvm::isa<llvm::ConstantInt>(actuals[idx])) {
            res.insert(formals[idx]); // corresponding formal
          }
        }
        if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
            llvm::isa<llvm::GlobalVariable>(source)) {
          res.insert(source);
        }
        return res;
      }
    };
    return make_shared<LCAFF>(llvm::ImmutableCallSite(callStmt), destFun);
  }
  // Pass everything else as identity
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::f_t calleeFun,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::n_t retSite) {
  // Handle the case: %x = call i32 ...
  if (callSite->getType()->isIntegerTy()) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(exitStmt);
    auto ReturnValue = Return->getReturnValue();
    struct LCAFF : FlowFunction<IDELinearConstantAnalysis::d_t> {
      IDELinearConstantAnalysis::n_t callSite;
      IDELinearConstantAnalysis::d_t ReturnValue;
      LCAFF(IDELinearConstantAnalysis::n_t cs,
            IDELinearConstantAnalysis::d_t retVal)
          : callSite(cs), ReturnValue(retVal) {}
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        // Collect return value fact
        if (source == ReturnValue) {
          res.insert(callSite);
        }
        // Return value is integer literal
        if (LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
            llvm::isa<llvm::ConstantInt>(ReturnValue)) {
          res.insert(callSite);
        }
        if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(source) &&
            llvm::isa<llvm::GlobalVariable>(source)) {
          res.insert(source);
        }
        return res;
      }
    };
    return make_shared<LCAFF>(callSite, ReturnValue);
  }
  // All other facts except GlobalVariables are killed at this point
  return make_shared<KillIf<IDELinearConstantAnalysis::d_t>>(
      [](IDELinearConstantAnalysis::d_t source) {
        return !llvm::isa<llvm::GlobalVariable>(source);
      });
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallToRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::n_t retSite, set<f_t> callees) {
  for (auto callee : callees) {
    if (!ICF->getStartPointsOf(callee).empty()) {
      return make_shared<KillIf<IDELinearConstantAnalysis::d_t>>(
          [this](IDELinearConstantAnalysis::d_t source) {
            return !isZeroValue(source) &&
                   llvm::isa<llvm::GlobalVariable>(source);
          });
    } else {
      return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
    }
  }
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getSummaryFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::f_t destFun) {
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
    IDELinearConstantAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// In addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(
    IDELinearConstantAnalysis::n_t curr,
    IDELinearConstantAnalysis::d_t currNode,
    IDELinearConstantAnalysis::n_t succ,
    IDELinearConstantAnalysis::d_t succNode) {
  auto &lg = lg::get();
  // Initialize global variables at entry point
  if (!isZeroValue(currNode) && ICF->isStartPoint(curr) &&
      isEntryPoint(ICF->getFunctionOf(curr)->getName().str()) &&
      llvm::isa<llvm::GlobalVariable>(currNode) && currNode == succNode) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Case: Intialize global variable at entry point.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    auto GV = llvm::dyn_cast<llvm::GlobalVariable>(currNode);
    auto CI = llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer());
    auto IntConst = CI->getSExtValue();
    return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
  }

  // All_Bottom for zero value
  if ((isZeroValue(currNode) && isZeroValue(succNode)) ||
      (llvm::isa<llvm::AllocaInst>(curr) && isZeroValue(currNode))) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Zero value.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
        IDELinearConstantAnalysis::BOTTOM);
  }

  // Check store instruction
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
    if (pointerOperand == succNode) {
      // Case I: Storing a constant integer.
      if (isZeroValue(currNode) && llvm::isa<llvm::ConstantInt>(valueOperand)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing constant integer.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand);
        auto IntConst = CI->getSExtValue();
        return make_shared<IDELinearConstantAnalysis::GenConstant>(IntConst);
      }
      // Case II: Storing an integer typed value.
      if (currNode != succNode && valueOperand->getType()->isIntegerTy()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing an integer typed value.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        return make_shared<IDELinearConstantAnalysis::LCAIdentity>();
      }
    }
  }

  // Check load instruction
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load == succNode) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Case: Loading an integer typed value.");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return make_shared<IDELinearConstantAnalysis::LCAIdentity>();
    }
  }

  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (curr == succNode && llvm::isa<llvm::BinaryOperator>(curr)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Binary operation.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    unsigned OP = curr->getOpcode();
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    // For non linear constant computation we propagate bottom
    if (currNode == ZeroValue && !llvm::isa<llvm::ConstantInt>(lop) &&
        !llvm::isa<llvm::ConstantInt>(rop)) {
      return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
          IDELinearConstantAnalysis::BOTTOM);
    } else {
      return make_shared<IDELinearConstantAnalysis::BinOp>(OP, lop, rop,
                                                           currNode);
    }
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Edge identity.");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getCallEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t srcNode,
    IDELinearConstantAnalysis::f_t destinationFunction,
    IDELinearConstantAnalysis::d_t destNode) {
  // Case: Passing constant integer as parameter
  if (isZeroValue(srcNode) && !isZeroValue(destNode)) {
    if (auto A = llvm::dyn_cast<llvm::Argument>(destNode)) {
      llvm::ImmutableCallSite CS(callStmt);
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
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::f_t calleeFunction,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::d_t exitNode,
    IDELinearConstantAnalysis::n_t reSite,
    IDELinearConstantAnalysis::d_t retNode) {
  // Case: Returning constant integer
  if (isZeroValue(exitNode) && !isZeroValue(retNode)) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(exitStmt);
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
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode,
    set<IDELinearConstantAnalysis::f_t> callees) {
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::getSummaryEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::l_t>::getInstance();
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::topElement() {
  return TOP;
}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::bottomElement() {
  return BOTTOM;
}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::join(IDELinearConstantAnalysis::l_t lhs,
                                IDELinearConstantAnalysis::l_t rhs) {
  if (lhs == TOP && rhs != BOTTOM) {
    return rhs;
  } else if (rhs == TOP && lhs != BOTTOM) {
    return lhs;
  } else if (rhs == lhs) {
    return rhs;
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
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> secondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LCAID = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return F->composeWith(G->composeWith(secondFunction));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          otherFunction.get())) {
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
    IDELinearConstantAnalysis::l_t source) {
  return IntConst;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> secondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDELinearConstantAnalysis::LCAEdgeFunctionComposer>(
      this->shared_from_this(), secondFunction);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::GenConstant::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::GenConstant::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> other) const {
  if (auto *GC =
          dynamic_cast<IDELinearConstantAnalysis::GenConstant *>(other.get())) {
    return (GC->IntConst == this->IntConst);
  }
  return this == other.get();
}

void IDELinearConstantAnalysis::GenConstant::print(ostream &OS,
                                                   bool isForDebug) const {
  OS << IntConst << " (EF:" << GenConstant_Id << ')';
}

IDELinearConstantAnalysis::LCAIdentity::LCAIdentity()
    : LCAID_Id(++IDELinearConstantAnalysis::CurrLCAID_Id) {}

IDELinearConstantAnalysis::l_t
IDELinearConstantAnalysis::LCAIdentity::computeTarget(
    IDELinearConstantAnalysis::l_t source) {
  return source;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> secondFunction) {
  return secondFunction;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::LCAIdentity::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::LCAIdentity::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> other) const {
  return this == other.get();
}

void IDELinearConstantAnalysis::LCAIdentity::print(ostream &OS,
                                                   bool isForDebug) const {
  OS << "Id (EF:" << LCAID_Id << ')';
}

IDELinearConstantAnalysis::BinOp::BinOp(const unsigned Op,
                                        IDELinearConstantAnalysis::d_t lop,
                                        IDELinearConstantAnalysis::d_t rop,
                                        IDELinearConstantAnalysis::d_t currNode)
    : EdgeFunctionID(++IDELinearConstantAnalysis::CurrBinary_Id), Op(Op),
      lop(lop), rop(rop), currNode(currNode) {}

IDELinearConstantAnalysis::l_t IDELinearConstantAnalysis::BinOp::computeTarget(
    IDELinearConstantAnalysis::l_t source) {
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
  } else if (source == BOTTOM) {
    return BOTTOM;
  } else if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
    auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
    return IDELinearConstantAnalysis::executeBinOperation(Op, source,
                                                          ric->getSExtValue());
  } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
    auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
    return IDELinearConstantAnalysis::executeBinOperation(
        Op, lic->getSExtValue(), source);
  }
  throw runtime_error("Only linear constant propagation can be specified!");
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> secondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<IDELinearConstantAnalysis::LCAIdentity *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDELinearConstantAnalysis::LCAEdgeFunctionComposer>(
      this->shared_from_this(), secondFunction);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>>
IDELinearConstantAnalysis::BinOp::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::l_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::l_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::BinOp::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::l_t>> other) const {
  if (auto *BOP =
          dynamic_cast<IDELinearConstantAnalysis::BinOp *>(other.get())) {
    return BOP->Op == this->Op && BOP->lop == this->lop &&
           BOP->rop == this->rop;
  }
  return this == other.get();
}

void IDELinearConstantAnalysis::BinOp::print(ostream &OS,
                                             bool isForDebug) const {
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

char IDELinearConstantAnalysis::opToChar(const unsigned op) {
  char opAsChar;
  switch (op) {
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
    const unsigned op, IDELinearConstantAnalysis::l_t lop,
    IDELinearConstantAnalysis::l_t rop) {
  // default initialize with BOTTOM (all information)
  IDELinearConstantAnalysis::l_t res = BOTTOM;
  switch (op) {
  case llvm::Instruction::Add:
    res = lop + rop;
    break;

  case llvm::Instruction::Sub:
    res = lop - rop;
    break;

  case llvm::Instruction::Mul:
    res = lop * rop;
    break;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    res = lop / rop;
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    res = lop % rop;
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
    ostream &os, IDELinearConstantAnalysis::n_t n) const {
  os << llvmIRToShortString(n);
}

void IDELinearConstantAnalysis::printDataFlowFact(
    ostream &os, IDELinearConstantAnalysis::d_t d) const {
  os << llvmIRToShortString(d);
}

void IDELinearConstantAnalysis::printFunction(
    ostream &os, IDELinearConstantAnalysis::f_t m) const {
  os << m->getName().str();
}

void IDELinearConstantAnalysis::printEdgeFact(
    ostream &os, IDELinearConstantAnalysis::l_t l) const {
  if (l == BOTTOM) {
    os << "Bottom";
  } else if (l == TOP) {
    os << "Top";
  } else {
    os << std::to_string(l);
  }
}

void IDELinearConstantAnalysis::emitTextReport(
    const SolverResults<IDELinearConstantAnalysis::n_t,
                        IDELinearConstantAnalysis::d_t,
                        IDELinearConstantAnalysis::l_t> &SR,
    std::ostream &os) {
  os << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    os << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (auto f : ICF->getAllFunctions()) {
      std::string fName = getFunctionNameFromIR(f);
      os << "\nFunction: " << fName << "\n----------"
         << std::string(fName.size(), '-') << '\n';
      for (auto stmt : ICF->getAllInstructionsOf(f)) {
        auto results = SR.resultsAt(stmt, true);
        stripBottomResults(results);
        if (!results.empty()) {
          os << "At IR statement: " << NtoString(stmt) << '\n';
          for (auto res : results) {
            if (res.second != IDELinearConstantAnalysis::BOTTOM) {
              os << "   Fact: " << DtoString(res.first)
                 << "\n  Value: " << LtoString(res.second) << '\n';
            }
          }
          os << '\n';
        }
      }
      os << '\n';
    }
  } else {
    auto lcaResults = getLCAResults(SR);
    for (auto entry : lcaResults) {
      os << "\nFunction: " << entry.first
         << "\n==========" << std::string(entry.first.size(), '=') << '\n';
      for (auto fResult : entry.second) {
        fResult.second.print(os);
        os << "--------------------------------------\n\n";
      }
      os << '\n';
    }
  }
}

void IDELinearConstantAnalysis::stripBottomResults(
    std::unordered_map<IDELinearConstantAnalysis::d_t,
                       IDELinearConstantAnalysis::l_t> &res) {
  for (auto it = res.begin(); it != res.end();) {
    if (it->second == IDELinearConstantAnalysis::BOTTOM) {
      it = res.erase(it);
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

void IDELinearConstantAnalysis::LCAResult::print(std::ostream &os) {
  os << "Line " << line_nr << ": " << src_code << '\n';
  os << "Var(s): ";
  for (auto it = variableToValue.begin(); it != variableToValue.end(); ++it) {
    if (it != variableToValue.begin()) {
      os << ", ";
    }
    os << it->first << " = " << it->second;
  }
  os << "\nCorresponding IR Instructions:\n";
  for (auto ir : ir_trace) {
    os << "  " << llvmIRToString(ir) << '\n';
  }
}

} // namespace psr
