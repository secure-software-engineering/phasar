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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <utility>

using namespace std;
using namespace psr;

namespace psr {

const IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::TOP =
    std::numeric_limits<IDELinearConstantAnalysis::v_t>::min();

const IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::BOTTOM =
    std::numeric_limits<IDELinearConstantAnalysis::v_t>::max();

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
      IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
      // kill pointer operand and only generate it, if value operand is valid!
      // necessary to ensure correctness
      struct LCAFF : FlowFunction<const llvm::Value *> {
        IDELinearConstantAnalysis::d_t pointerOperand;
        IDELinearConstantAnalysis::d_t valueOperand;
        LCAFF(IDELinearConstantAnalysis::d_t pOp,
              IDELinearConstantAnalysis::d_t vOp)
            : pointerOperand(pOp), valueOperand(vOp) {}
        set<IDELinearConstantAnalysis::d_t>
        computeTargets(IDELinearConstantAnalysis::d_t source) override {
          if (source == pointerOperand) {
            return {};
          } else if (source == valueOperand) {
            return {source, pointerOperand};
          } else {
            return {source};
          }
        }
      };
      return make_shared<LCAFF>(pointerOperand, valueOperand);
      // return make_shared<GenIf<IDELinearConstantAnalysis::d_t>>(
      //     pointerOperand, zeroValue(),
      //     [Store](IDELinearConstantAnalysis::d_t source) {
      //       return source == Store->getValueOperand();
      //     });
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
      LCAFF(llvm::ImmutableCallSite callSite,
            IDELinearConstantAnalysis::m_t destMthd) {
        // set up the actual parameters
        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
          actuals.push_back(callSite.getArgOperand(idx));
        }
        // set up the formal parameters
        for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destMthd, idx));
        }
      }
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        for (unsigned idx = 0; idx < actuals.size(); ++idx) {
          // ordinary case: just perform mapping
          if (source == actuals[idx]) {
            res.insert(formals[idx]); // corresponding formal
          }
          // special case: check if function is called with integer literals as
          // parameter
          if (isLLVMZeroValue(source) &&
              llvm::isa<llvm::ConstantInt>(actuals[idx])) {
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
    struct LCAFF : FlowFunction<IDELinearConstantAnalysis::d_t> {
      IDELinearConstantAnalysis::n_t callSite;
      IDELinearConstantAnalysis::d_t ReturnValue;
      LCAFF(IDELinearConstantAnalysis::n_t cs,
            IDELinearConstantAnalysis::d_t retVal)
          : callSite(cs), ReturnValue(retVal) {}
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        // collect return value fact
        if (source == ReturnValue) {
          res.insert(callSite);
        }
        // return value is integer literal
        if (isLLVMZeroValue(source) &&
            llvm::isa<llvm::ConstantInt>(ReturnValue)) {
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
  // all bottom for zero value
  if (isZeroValue(currNode) && isZeroValue(succNode)) {
    return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
        bottomElement());
  }
  // check store instructions
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
    if (pointerOperand == succNode /*&& isZeroValue(currNode)*/) {
      // case i: storing constant integer
      if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand)) {
        auto iconst = CI->getSExtValue();
        struct LCAEF : EdgeFunction<IDELinearConstantAnalysis::v_t>,
                       enable_shared_from_this<LCAEF> {
          IDELinearConstantAnalysis::v_t iconst;
          LCAEF(IDELinearConstantAnalysis::v_t ic) : iconst(ic) {}

          IDELinearConstantAnalysis::v_t
          computeTarget(IDELinearConstantAnalysis::v_t source) override {
            return iconst;
          }

          shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
          composeWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                          secondFunction) override {
            return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
                this->shared_from_this(), secondFunction);
          }

          shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
          joinWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                       otherFunction) override {
            auto thisF =
                dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(this);
            auto otherF =
                dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(
                    otherFunction.get());
            if (thisF && !otherF) {
              return otherFunction;
            } else if (!thisF && otherF) {
              return this->shared_from_this();
            } else {
              return this->shared_from_this();
            }
          }

          bool equal_to(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                            other) const override {
            return this == other.get();
          }
        };
        return make_shared<LCAEF>(iconst);
      }
      // case ii: storing integer type value
      if (valueOperand->getType()->isIntegerTy()) {
        struct LCAEF : EdgeIdentity<IDELinearConstantAnalysis::v_t> {
          shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
          composeWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                          secondFunction) override {
            return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
                this->shared_from_this(), secondFunction);
          }

          shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
          joinWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                       otherFunction) override {
            auto thisF =
                dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(this);
            auto otherF =
                dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(
                    otherFunction.get());
            if (thisF && !otherF) {
              return otherFunction;
            } else if (!thisF && otherF) {
              return this->shared_from_this();
            } else {
              return this->shared_from_this();
            }
          }

          bool equal_to(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                            other) const override {
            return this == other.get();
          }
        };
        return LCAEF::getInstance();
        // return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
      }
    }
  }

  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load == succNode) {
      struct LCAEF : EdgeIdentity<IDELinearConstantAnalysis::v_t> {
        shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
        composeWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                        secondFunction) override {
          return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
              this->shared_from_this(), secondFunction);
        }

        shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
        joinWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                     otherFunction) override {
          auto thisF =
              dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(this);
          auto otherF =
              dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(
                  otherFunction.get());
          if (thisF && !otherF) {
            return otherFunction;
          } else if (!thisF && otherF) {
            return this->shared_from_this();
          } else {
            return this->shared_from_this();
          }
        }

        bool equal_to(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                          other) const override {
          return this == other.get();
        }
      };
      return LCAEF::getInstance();
      // return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (curr == succNode && llvm::isa<llvm::BinaryOperator>(curr)) {
    unsigned op = curr->getOpcode();
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    struct LCAEF : EdgeFunction<IDELinearConstantAnalysis::v_t>,
                   enable_shared_from_this<LCAEF> {
      unsigned op;
      IDELinearConstantAnalysis::d_t lop, rop, currNode;
      LCAEF(unsigned op, IDELinearConstantAnalysis::d_t lop,
            IDELinearConstantAnalysis::d_t rop,
            IDELinearConstantAnalysis::d_t currNode)
          : op(op), lop(lop), rop(rop), currNode(currNode) {}

      IDELinearConstantAnalysis::v_t
      computeTarget(IDELinearConstantAnalysis::v_t source) override {
        cout << "LOP:" << endl;
        lop->print(llvm::outs());
        cout << "\nROP:" << endl;
        rop->print(llvm::outs());
        cout << "\nCURR NODE:" << endl;
        currNode->print(llvm::outs());
        cout << endl;
        if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
          cout << "lop == currNode AND rop is iconst" << endl;
          auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
          return IDELinearConstantAnalysis::executeBinOperation(
              op, source, ric->getSExtValue());
        } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
          cout << "rop == currNode AND lop is iconst" << endl;
          auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
          return IDELinearConstantAnalysis::executeBinOperation(
              op, lic->getSExtValue(), source);
        }
        throw runtime_error(
            "Only linear constant propagation can be specified!");
      }

      shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
      composeWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                      secondFunction) override {
        return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
            this->shared_from_this(), secondFunction);
      }

      shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
      joinWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                   otherFunction) override {
        auto thisF =
            dynamic_cast<AllBottom<IDELinearConstantAnalysis::v_t> *>(this);
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

      bool equal_to(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                        other) const override {
        return this == other.get();
      }
    };
    return make_shared<LCAEF>(op, lop, rop, currNode);
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
  return G->computeTarget(F->computeTarget(source));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::EdgeFunctionComposer::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> secondFunction) {
  return F->composeWith(G->composeWith(secondFunction));
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

bool IDELinearConstantAnalysis::EdgeFunctionComposer::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> other) const {
  return G->equal_to(other);
}

// add, sub, mul, udiv/sdiv, urem/srem
IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::executeBinOperation(
    unsigned op, IDELinearConstantAnalysis::v_t lop,
    IDELinearConstantAnalysis::v_t rop) {
  IDELinearConstantAnalysis::v_t res;
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
    throw runtime_error("Could not execute unknown operation '" +
                        to_string(op) + "'!");
  }
  return res;
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
