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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
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
// Initialize debug counter for edge functions
unsigned IDELinearConstantAnalysis::StoreConstCount = 0;
unsigned IDELinearConstantAnalysis::LoadStoreValueCount = 0;
unsigned IDELinearConstantAnalysis::BinaryCount = 0;
unsigned IDELinearConstantAnalysis::EdgeComposerCount = 0;

const IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::TOP =
    std::numeric_limits<IDELinearConstantAnalysis::v_t>::min();

const IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::BOTTOM =
    std::numeric_limits<IDELinearConstantAnalysis::v_t>::max();

IDELinearConstantAnalysis::IDELinearConstantAnalysis(
    IDELinearConstantAnalysis::i_t &icfg, vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

IDELinearConstantAnalysis::~IDELinearConstantAnalysis() {
  IDELinearConstantAnalysis::StoreConstCount = 0;
  IDELinearConstantAnalysis::LoadStoreValueCount = 0;
  IDELinearConstantAnalysis::BinaryCount = 0;
  IDELinearConstantAnalysis::EdgeComposerCount = 0;
}

// Start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getNormalFlowFunction(
    IDELinearConstantAnalysis::n_t curr, IDELinearConstantAnalysis::n_t succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getNormalFlowFunction()";
  // Check commandline arguments, e.g. argc, and generate all integer
  // typed arguments.
  // if (curr->getFunction()->getName().str() == "main" &&
  //     icfg.isStartPoint(curr)) {
  //   set<IDELinearConstantAnalysis::d_t> CmdArgs;
  //   for (auto &Arg : curr->getFunction()->args()) {
  //     if (Arg.getType()->isIntegerTy()) {
  //       CmdArgs.insert(&Arg);
  //       if (bl::core::get()->get_logging_enabled()) {
  //         std::string cmdarg_str;
  //         llvm::raw_string_ostream rso(cmdarg_str);
  //         Arg.print(rso);
  //         BOOST_LOG_SEV(lg, DEBUG) << "CmdArg: " << rso.str();
  //       }
  //     }
  //   }
  //   BOOST_LOG_SEV(lg, DEBUG) << ' ';
  //   return make_shared<GenAll<IDELinearConstantAnalysis::d_t>>(CmdArgs,
  //                                                              zeroValue());
  // }
  // BOOST_LOG_SEV(lg, DEBUG) << ' ';
  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e. kills previous fact (= pointer operand).
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t PointerOp = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      struct LCAFF : FlowFunction<IDELinearConstantAnalysis::d_t> {
        IDELinearConstantAnalysis::d_t PointerOp, ZeroValue;
        LCAFF(IDELinearConstantAnalysis::d_t PointerOperand,
              IDELinearConstantAnalysis::d_t ZeroValue)
            : PointerOp(PointerOperand), ZeroValue(ZeroValue) {}
        set<IDELinearConstantAnalysis::d_t>
        computeTargets(IDELinearConstantAnalysis::d_t source) override {
          if (source == PointerOp) {
            return {};
          } else if (source == ZeroValue) {
            return {source, PointerOp};
          } else {
            return {source};
          }
        }
      };
      return make_shared<LCAFF>(PointerOp, zeroValue());
    }
    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      struct LCAFF : FlowFunction<IDELinearConstantAnalysis::d_t> {
        IDELinearConstantAnalysis::d_t PointerOp, ValueOp;
        LCAFF(IDELinearConstantAnalysis::d_t PointerOperand,
              IDELinearConstantAnalysis::d_t ValueOperand)
            : PointerOp(PointerOperand), ValueOp(ValueOperand) {}
        set<IDELinearConstantAnalysis::d_t>
        computeTargets(IDELinearConstantAnalysis::d_t source) override {
          if (source == PointerOp) {
            return {};
          } else if (source == ValueOp) {
            return {source, PointerOp};
          } else {
            return {source};
          }
        }
      };
      return make_shared<LCAFF>(PointerOp, ValueOp);
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
          return (source != zerovalue &&
                  ((lop == source && llvm::isa<llvm::ConstantInt>(rop)) ||
                   (rop == source && llvm::isa<llvm::ConstantInt>(lop)))) ||
                 (source == zerovalue && llvm::isa<llvm::ConstantInt>(lop) &&
                  llvm::isa<llvm::ConstantInt>(rop));
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
  BOOST_LOG_SEV(lg, DEBUG) << ' ';
  // Map the actual parameters into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    struct LCAFF : FlowFunction<const llvm::Value *> {
      vector<const llvm::Value *> actuals;
      vector<const llvm::Value *> formals;
      LCAFF(llvm::ImmutableCallSite callSite,
            IDELinearConstantAnalysis::m_t destMthd) {
        // Set up the actual parameters
        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
          actuals.push_back(callSite.getArgOperand(idx));
        }
        // Set up the formal parameters
        for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destMthd, idx));
        }
      }
      set<IDELinearConstantAnalysis::d_t>
      computeTargets(IDELinearConstantAnalysis::d_t source) override {
        set<IDELinearConstantAnalysis::d_t> res;
        for (unsigned idx = 0; idx < actuals.size(); ++idx) {
          // Ordinary case: Just perform mapping
          if (source == actuals[idx]) {
            res.insert(formals[idx]); // corresponding formal
          }
          // Special case: Check if function is called with integer literals as
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
  BOOST_LOG_SEV(lg, DEBUG) << ' ';
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
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getCallToRetFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << ' ';
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getSummaryFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getSummaryFlowFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << ' ';
  return nullptr;
}

map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::initialSeeds() {
  // Check commandline arguments, e.g. argc, and generate all integer
  // typed arguments.
  map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    if (EntryPoint == "main") {
      set<IDELinearConstantAnalysis::d_t> CmdArgs;
      for (auto &Arg : icfg.getMethod(EntryPoint)->args()) {
        if (Arg.getType()->isIntegerTy()) {
          CmdArgs.insert(&Arg);
        }
      }
      CmdArgs.insert(zeroValue());
      SeedMap.insert(
          make_pair(&icfg.getMethod(EntryPoint)->front().front(), CmdArgs));
    } else {
      SeedMap.insert(
          make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                    set<IDELinearConstantAnalysis::d_t>({zeroValue()})));
    }
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

// In addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(
    IDELinearConstantAnalysis::n_t curr,
    IDELinearConstantAnalysis::d_t currNode,
    IDELinearConstantAnalysis::n_t succ,
    IDELinearConstantAnalysis::d_t succNode) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IDELinearConstantAnalysis::getNormalEdgeFunction()";
  BOOST_LOG_SEV(lg, DEBUG) << "(N) Curr Inst : "
                           << IDELinearConstantAnalysis::NtoString(curr);
  BOOST_LOG_SEV(lg, DEBUG) << "(D) Curr Node :   "
                           << IDELinearConstantAnalysis::DtoString(currNode);
  BOOST_LOG_SEV(lg, DEBUG) << "(N) Succ Inst : "
                           << IDELinearConstantAnalysis::NtoString(succ);
  BOOST_LOG_SEV(lg, DEBUG) << "(D) Succ Node :   "
                           << IDELinearConstantAnalysis::DtoString(succNode);
  // All_Bottom for zero value
  if (isZeroValue(currNode) && isZeroValue(succNode)) {
    BOOST_LOG_SEV(lg, DEBUG) << "Case: Zero value.";
    BOOST_LOG_SEV(lg, DEBUG) << ' ';
    return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
        bottomElement());
  }
  // Check store instruction
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
    IDELinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
    if (pointerOperand == succNode) {
      // Case I: Storing a constant integer.
      if (isZeroValue(currNode) && llvm::isa<llvm::ConstantInt>(valueOperand)) {
        BOOST_LOG_SEV(lg, DEBUG) << "Case: Storing constant integer.";
        BOOST_LOG_SEV(lg, DEBUG) << ' ';
        auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand);
        auto IntConst = CI->getSExtValue();
        return make_shared<IDELinearConstantAnalysis::StoreConstant>(IntConst);
      }
      // Case II: Storing an integer typed value.
      if (currNode != succNode && valueOperand->getType()->isIntegerTy()) {
        BOOST_LOG_SEV(lg, DEBUG) << "Case: Storing an integer typed value.";
        BOOST_LOG_SEV(lg, DEBUG) << ' ';
        return make_shared<IDELinearConstantAnalysis::LoadStoreValueIdentity>();
      }
    }
  }

  // Check load instruction
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load == succNode) {
      BOOST_LOG_SEV(lg, DEBUG) << "Case: Loading an integer typed value.";
      BOOST_LOG_SEV(lg, DEBUG) << ' ';
      return make_shared<IDELinearConstantAnalysis::LoadStoreValueIdentity>();
    }
  }
  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (curr == succNode && llvm::isa<llvm::BinaryOperator>(curr)) {
    BOOST_LOG_SEV(lg, DEBUG) << "Case: Binary operation.";
    BOOST_LOG_SEV(lg, DEBUG) << ' ';
    unsigned OP = curr->getOpcode();
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    struct LCAEF : EdgeFunction<IDELinearConstantAnalysis::v_t>,
                   enable_shared_from_this<LCAEF> {
      const unsigned EdgeFunctionID, Op;
      IDELinearConstantAnalysis::d_t lop, rop, currNode;
      LCAEF(const unsigned Op, IDELinearConstantAnalysis::d_t lop,
            IDELinearConstantAnalysis::d_t rop,
            IDELinearConstantAnalysis::d_t currNode)
          : EdgeFunctionID(++IDELinearConstantAnalysis::BinaryCount), Op(Op),
            lop(lop), rop(rop), currNode(currNode) {}

      IDELinearConstantAnalysis::v_t
      computeTarget(IDELinearConstantAnalysis::v_t source) override {
        auto &lg = lg::get();
        BOOST_LOG_SEV(lg, DEBUG) << "Left Op   : " << llvmIRToString(lop);
        BOOST_LOG_SEV(lg, DEBUG) << "Right Op  : " << llvmIRToString(rop);
        BOOST_LOG_SEV(lg, DEBUG) << "Curr Node : " << llvmIRToString(currNode);
        BOOST_LOG_SEV(lg, DEBUG) << ' ';
        if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
          auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
          return IDELinearConstantAnalysis::executeBinOperation(
              Op, source, ric->getSExtValue());
        } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
          auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
          return IDELinearConstantAnalysis::executeBinOperation(
              Op, lic->getSExtValue(), source);
        } else if (isLLVMZeroValue(currNode) &&
                   llvm::isa<llvm::ConstantInt>(lop) &&
                   llvm::isa<llvm::ConstantInt>(rop)) {
          auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
          auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
          return IDELinearConstantAnalysis::executeBinOperation(
              Op, lic->getSExtValue(), ric->getSExtValue());
        }
        throw runtime_error(
            "Only linear constant propagation can be specified!");
      }

      shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
      composeWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                      secondFunction) override {
        if (auto *EI =
                dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::v_t> *>(
                    secondFunction.get())) {
          return this->shared_from_this();
        }
        if (auto *LSVI =
                dynamic_cast<LoadStoreValueIdentity *>(secondFunction.get())) {
          return this->shared_from_this();
        }
        return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
            this->shared_from_this(), secondFunction);
      }

      shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
      joinWith(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                   otherFunction) override {
        if (otherFunction.get() == this ||
            otherFunction->equal_to(this->shared_from_this())) {
          return this->shared_from_this();
        }
        if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::v_t> *>(
                otherFunction.get())) {
          return this->shared_from_this();
        }
        return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
            IDELinearConstantAnalysis::BOTTOM);
      }

      bool equal_to(shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
                        other) const override {
        // could be more precise - compare op,lop and rop?
        return this == other.get();
      }

      virtual void print(std::ostream &OS,
                         bool isForDebug = false) const override {
        OS << "Binary_" << EdgeFunctionID;
      }
    };
    return make_shared<LCAEF>(OP, lop, rop, currNode);
  }
  BOOST_LOG_SEV(lg, DEBUG) << "Case: Edge identity.";
  BOOST_LOG_SEV(lg, DEBUG) << ' ';
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
  } else if (rhs == lhs) {
    return rhs;
  } else {
    return BOTTOM;
  }
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::allTopFunction() {
  return make_shared<AllTop<IDELinearConstantAnalysis::v_t>>(TOP);
}

IDELinearConstantAnalysis::EdgeFunctionComposer::EdgeFunctionComposer(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> F,
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> G)
    : EdgeFunctionID(++IDELinearConstantAnalysis::EdgeComposerCount), F(F),
      G(G) {}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::EdgeFunctionComposer::computeTarget(
    IDELinearConstantAnalysis::v_t source) {
  return G->computeTarget(F->computeTarget(source));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::EdgeFunctionComposer::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::v_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI =
          dynamic_cast<LoadStoreValueIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return F->composeWith(G->composeWith(secondFunction));
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::EdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::v_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::EdgeFunctionComposer::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> other) const {
  if (auto EC = dynamic_cast<EdgeFunctionComposer *>(other.get())) {
    return F->equal_to(EC->F) && G->equal_to(EC->G);
  }
  return false;
}

void IDELinearConstantAnalysis::EdgeFunctionComposer::print(
    std::ostream &OS, bool isForDebug) const {
  OS << "EC_" << IDELinearConstantAnalysis::EdgeFunctionComposer::EdgeFunctionID
     << "[ " << F.get()->str() << " , " << G.get()->str() << " ]";
}

IDELinearConstantAnalysis::StoreConstant::StoreConstant(
    IDELinearConstantAnalysis::v_t IntConst)
    : EdgeFunctionID(++IDELinearConstantAnalysis::StoreConstCount),
      IntConst(IntConst) {}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::StoreConstant::computeTarget(
    IDELinearConstantAnalysis::v_t source) {
  return IntConst;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::StoreConstant::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<IDELinearConstantAnalysis::v_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI =
          dynamic_cast<LoadStoreValueIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDELinearConstantAnalysis::EdgeFunctionComposer>(
      this->shared_from_this(), secondFunction);
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::StoreConstant::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::StoreConstant::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> other) const {
  if (auto *StC = dynamic_cast<IDELinearConstantAnalysis::StoreConstant *>(
          other.get())) {
    return (StC->IntConst == this->IntConst);
  }
  return this == other.get();
}

void IDELinearConstantAnalysis::StoreConstant::print(std::ostream &OS,
                                                     bool isForDebug) const {
  OS << "StoreConst_" << EdgeFunctionID;
}

IDELinearConstantAnalysis::LoadStoreValueIdentity::LoadStoreValueIdentity()
    : EdgeFunctionID(++IDELinearConstantAnalysis::LoadStoreValueCount) {}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::LoadStoreValueIdentity::computeTarget(
    IDELinearConstantAnalysis::v_t source) {
  return source;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::LoadStoreValueIdentity::composeWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> secondFunction) {
  return secondFunction;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::LoadStoreValueIdentity::joinWith(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDELinearConstantAnalysis::v_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDELinearConstantAnalysis::v_t>>(
      IDELinearConstantAnalysis::BOTTOM);
}

bool IDELinearConstantAnalysis::LoadStoreValueIdentity::equal_to(
    shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>> other) const {
  return this == other.get();
}

void IDELinearConstantAnalysis::LoadStoreValueIdentity::print(
    std::ostream &OS, bool isForDebug) const {
  OS << "Load/StoreValue_" << EdgeFunctionID;
}

// add, sub, mul, udiv/sdiv, urem/srem
IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::executeBinOperation(
    const unsigned op, IDELinearConstantAnalysis::v_t lop,
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
