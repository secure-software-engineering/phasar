/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <limits>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenIf.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <utility>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>

using namespace std;
using namespace psr;

namespace psr {

const int IDELinearConstantAnalysis::TOP = std::numeric_limits<int>::min();

const int IDELinearConstantAnalysis::BOTTOM = std::numeric_limits<int>::max();

IDELinearConstantAnalysis::IDELinearConstantAnalysis(
    IDELinearConstantAnalysis::i_t &icfg, vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getNormalFlowFunction(
    IDELinearConstantAnalysis::n_t curr, IDELinearConstantAnalysis::n_t succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getNormalFlowFunction()";
  // check store instructions
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    // value operand is a constant integer
    if (llvm::isa<llvm::ConstantInt>(Store->getValueOperand())) {
      return make_shared<Gen<IDELinearConstantAnalysis::d_t>>(pointerOperand,
                                                              zeroValue());
    }
    // value operand is integer type
    if (Store->getValueOperand()->getType()->isIntegerTy()) {
      return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
          pointerOperand, zeroValue(),
          [Store](IDELinearConstantAnalysis::d_t source) {
            return source == Store->getValueOperand();
          });
    }
  }
  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    // only consider i32 load
    if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
      return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
          Load, zeroValue(), [Load](IDELinearConstantAnalysis::d_t source) {
            return source == Load->getPointerOperand();
          });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(curr)) {
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
        curr, zeroValue(),
        [this, lop, rop](IDELinearConstantAnalysis::d_t source) {
          return source != zerovalue &&
                 ((lop == source && llvm::isa<llvm::ConstantInt>(rop)) ||
                  (rop == source && llvm::isa<llvm::ConstantInt>(lop)));
        });
  }
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getCallFlowFunction()";
  // map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    struct LCAFF : FlowFunction<const llvm::Value *> {
      vector<const llvm::Value *> actuals;
      vector<const llvm::Value *> formals;
      LCAFF(llvm::ImmutableCallSite callSite, IDELinearConstantAnalysis::m_t destMthd) {
        // set up the actual parameters
        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
          actuals.push_back(callSite.getArgOperand(idx));
        }
        // set up the formal parameters
        for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destMthd, idx));
        }
      }
      set<IDELinearConstantAnalysis::d_t> computeTargets(IDELinearConstantAnalysis::d_t source) {
        set<IDELinearConstantAnalysis::d_t> res;
        for (unsigned idx = 0; idx < actuals.size(); ++idx) {
          // ordinary case: just perform mapping
          if (source == actuals[idx]) {
            res.insert(formals[idx]); // corresponding formal
          }
          // special case: check if function is called with integer literals as parameter
          if (isLLVMZeroValue(source) && llvm::isa<llvm::ConstantInt>(actuals[idx])) {
            res.insert(formals[idx]); // corresponding formal
          }
        }
        return res;
      }
    };
    return make_shared<LCAFF>(llvm::ImmutableCallSite(callStmt), destMthd);
  }
  // Pass everything else as identity
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::m_t calleeMthd,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::n_t retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IDELinearConstantAnalysis::getRetFlowFunction()";
  // handle the case: %x = call i32 ...
  if (callSite->getType()->isIntegerTy()) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(exitStmt);
    auto ReturnValue = Return->getReturnValue();
    struct LCAFF : FlowFunction<const llvm::Value *> {
      IDELinearConstantAnalysis::n_t callSite;
      IDELinearConstantAnalysis::d_t ReturnValue;
      LCAFF(IDELinearConstantAnalysis::n_t cs, IDELinearConstantAnalysis::d_t retVal) : callSite(cs), ReturnValue(retVal) {}
      set<IDELinearConstantAnalysis::d_t> computeTargets(IDELinearConstantAnalysis::d_t source) {
        set<IDELinearConstantAnalysis::d_t> res;
        // collect return value fact
        if (source == ReturnValue) {
          res.insert(callSite);
        }
        // return value is integer literal
        if (isLLVMZeroValue(source) && llvm::isa<llvm::ConstantInt>(ReturnValue)) {
          res.insert(callSite);
        }
        return res;
      }
    };
    return make_shared<LCAFF>(callSite, ReturnValue);
  }
  // All other facts are killed at this point
  return KillAll<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallToRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::n_t retSite, std::set<m_t> callees) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IDELinearConstantAnalysis::getRetFlowFunction()";
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getSummaryFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getSummaryFlowFunction()";
  return nullptr;
}

map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::initialSeeds() {
  // just start in main()
  map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                       set<IDELinearConstantAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDELinearConstantAnalysis::d_t IDELinearConstantAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantAnalysis::isZeroValue(
    IDELinearConstantAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(
    IDELinearConstantAnalysis::n_t curr,
    IDELinearConstantAnalysis::d_t currNode,
    IDELinearConstantAnalysis::n_t succ,
    IDELinearConstantAnalysis::d_t succNode) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getNormalEdgeFunction()";
  if (isZeroValue(currNode) && isZeroValue(succNode)) {
    return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
        bottomElement());
  }
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
    if (pointerOperand == succNode) {
      if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand)) {
        auto iconst = CI->getSExtValue();
      }
    }
  }
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getCallEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t srcNode,
    IDELinearConstantAnalysis::m_t destiantionMethod,
    IDELinearConstantAnalysis::d_t destNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getReturnEdgeFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::m_t calleeMethod,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::d_t exitNode,
    IDELinearConstantAnalysis::n_t reSite,
    IDELinearConstantAnalysis::d_t retNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getCallToReturnEdgeFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getSummaryEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::topElement() {
  return TOP;
}

IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::bottomElement() {
  return BOTTOM;
}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::join(IDELinearConstantAnalysis::v_t lhs,
                                IDELinearConstantAnalysis::v_t rhs) {
  if (lhs == TOP && rhs != BOTTOM) {
    return rhs;
  } else if (rhs == TOP && lhs != BOTTOM) {
    return lhs;
    // } else if (rhs == lhs) {
    //   return rhs;
  } else {
    return BOTTOM;
  }
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::allTopFunction() {
  return make_shared<AllTop<IDELinearConstantAnalysis::v_t>>(TOP);
}

// Use std::move()?
IDELinearConstantAnalysis::EdgeFunctionComposer::EdgeFunctionComposer(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> f,
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> g)
    : F(f), G(g) {}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::EdgeFunctionComposer::computeTarget(
    IDELinearConstantAnalysis::v_t source) {
  return F->computeTarget(G->computeTarget(source));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::EdgeFunctionComposer::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> secondFunction) {
  return G->composeWith(F->composeWith(secondFunction));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::EdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> otherFunction) {
  auto thisF = dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(this);
  auto otherF = dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(
      otherFunction.get());
  if (thisF && !otherF) {
    return otherFunction;
  } else if (!thisF && otherF) {
    return this->shared_from_this();
  } else {
    return this->shared_from_this();
  }
}

bool IDELinearConstantAnalysis::EdgeFunctionComposer::equalTo(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> other) {
  return F->equalTo(other);
}

string
IDELinearConstantAnalysis::DtoString(IDELinearConstantAnalysis::d_t d) const {
  return llvmIRToString(d);
}

string
IDELinearConstantAnalysis::VtoString(IDELinearConstantAnalysis::v_t v) const {
  return to_string(v);
}

string
IDELinearConstantAnalysis::NtoString(IDELinearConstantAnalysis::n_t n) const {
  return llvmIRToString(n);
}

string
IDELinearConstantAnalysis::MtoString(IDELinearConstantAnalysis::m_t m) const {
  return m->getName().str();
}

} // namespace psr
