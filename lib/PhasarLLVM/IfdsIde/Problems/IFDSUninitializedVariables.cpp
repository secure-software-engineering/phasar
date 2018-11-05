/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>

#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

IFDSUnitializedVariables::IFDSUnitializedVariables(
    IFDSUnitializedVariables::i_t icfg, vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSUnitializedVariables::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getNormalFlowFunction(
    IFDSUnitializedVariables::n_t curr, IFDSUnitializedVariables::n_t succ) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getNormalFlowFunction()");
  // initially mark every local as uninitialized (except entry point args)
  if (icfg.isStartPoint(curr) &&
      curr->getFunction()->getName().str() == "main") {
    const llvm::Function *func = icfg.getMethodOf(curr);
    // set all locals as uninitialized flow function
    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Function *func;
      IFDSUnitializedVariables::d_t zerovalue;
      UVFF(const llvm::Function *f, IFDSUnitializedVariables::d_t zv)
          : func(f), zerovalue(zv) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        if (source == zerovalue) {
          set<IFDSUnitializedVariables::d_t> res;
          // first add all local values of primitive types
          for (auto &BB : *func) {
            for (auto &inst : BB) {
              // collect all alloca instructions of primitive types
              if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                if (alloc->getAllocatedType()->isIntegerTy() ||
                    alloc->getAllocatedType()->isFloatingPointTy() ||
                    alloc->getAllocatedType()->isPointerTy() ||
                    alloc->getAllocatedType()->isArrayTy()) {
                  res.insert(alloc);
                }
              } else {
                // collect all instructions that use an undef literal
                for (auto &operand : inst.operands()) {
                  if (llvm::isa<llvm::UndefValue>(&operand)) {
                    res.insert(&inst);
                  }
                }
              }
            }
          }
          // remove function parameters of entry function
          for (auto &arg : func->args()) {
            res.erase(&arg);
          }
          res.insert(zerovalue);
          return res;
        }
        return {};
      }
    };
    return make_shared<UVFF>(func, zerovalue);
  }

  // check the all store instructions and kill initialized variables
  if (auto store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Value *valueop;
      const llvm::Value *pointerop;
      const llvm::StoreInst *store;
      map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
          &UndefValueUses;
      UVFF(const llvm::StoreInst *s,
           map<IFDSUnitializedVariables::n_t,
               set<IFDSUnitializedVariables::d_t>> &UVU)
          : store(s), UndefValueUses(UVU) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        // check if an uninitialized value is loaded and stored in a variable
        for (auto &use : store->getValueOperand()->uses()) {
          // check if use is load
          if (const llvm::LoadInst *load =
                  llvm::dyn_cast<llvm::LoadInst>(use)) {
            // if the following is uninit, then this store must be uninit
            if (source == load->getPointerOperand() || source == load) {
              UndefValueUses[load].insert(load->getPointerOperand());
              return {source, load, store->getValueOperand(),
                      store->getPointerOperand()};
            }
          }
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(use)) {
            for (auto &operand : inst->operands()) {
              if (const llvm::Value *val =
                      llvm::dyn_cast<llvm::Value>(&operand)) {
                if (val == source || llvm::isa<llvm::UndefValue>(val)) {
                  return {source, val, store->getPointerOperand()};
                }
              }
            }
          }
          if (use.get() == source) {
            return {source, store->getValueOperand(),
                    store->getPointerOperand()};
          }
        }
        // otherwise initialize (kill) the value
        if (store->getPointerOperand() == source) {
          return {};
        }
        // pass all other facts as identity
        return {source};
      }
    };
    return make_shared<UVFF>(store, UndefValueUses);
  }

  // check if some instruction is using an undefined value (in)directly
  struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
    const llvm::Instruction *inst;
    map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
        &UndefValueUses;
    UVFF(const llvm::Instruction *inst,
         map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
             &UVU)
        : inst(inst), UndefValueUses(UVU) {}
    set<IFDSUnitializedVariables::d_t>
    computeTargets(IFDSUnitializedVariables::d_t source) override {
      for (auto &operand : inst->operands()) {
        const llvm::UndefValue *undef =
            llvm::dyn_cast<llvm::UndefValue>(operand);
        if (operand == source || operand == undef) {
          UndefValueUses[inst].insert(operand);
          return {source, inst};
        }
      }
      return {source};
    }
  };
  return make_shared<UVFF>(curr, UndefValueUses);

  // otherwise we do not care and nothing changes
  return Identity<IFDSUnitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getCallFlowFunction(
    IFDSUnitializedVariables::n_t callStmt,
    IFDSUnitializedVariables::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getCallFlowFunction()");
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    llvm::ImmutableCallSite callSite(callStmt);
    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Function *destMthd;
      llvm::ImmutableCallSite callSite;
      const llvm::Value *zerovalue;
      vector<const llvm::Value *> actuals;
      vector<const llvm::Value *> formals;
      UVFF(const llvm::Function *dm, llvm::ImmutableCallSite cs,
           const llvm::Value *zv)
          : destMthd(dm), callSite(cs), zerovalue(zv) {
        // set up the actual parameters
        for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
          actuals.push_back(callSite.getArgOperand(idx));
        }
        // set up the formal parameters
        for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destMthd, idx));
        }
      }

      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        // perform parameter passing
        if (source != zerovalue) {
          set<const llvm::Value *> res;
          // do the mapping from actual to formal parameters
          // caution: the loop iterates from 0 to formals.size(),
          // rather than actuals.size() as we may have more actual
          // than formal arguments in case of C-style varargs
          for (unsigned idx = 0; idx < formals.size(); ++idx) {
            if (source == actuals[idx]) {
              res.insert(formals[idx]);
            }
          }
          return res;
        } else {
          // on zerovalue -> gen all locals parameter
          set<const llvm::Value *> res;
          for (auto &BB : *destMthd) {
            for (auto &inst : BB) {
              if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                // check if the allocated value is of a primitive type
                if (alloc->getAllocatedType()->isIntegerTy() ||
                    alloc->getAllocatedType()->isFloatingPointTy() ||
                    alloc->getAllocatedType()->isPointerTy() ||
                    alloc->getAllocatedType()->isArrayTy()) {
                  res.insert(alloc);
                }
              } else {
                // check for instructions using undef value directly
                for (auto &operand : inst.operands()) {
                  if (llvm::isa<llvm::UndefValue>(&operand)) {
                    res.insert(&inst);
                  }
                }
              }
            }
          }
          return res;
        }
      }
    };
    return make_shared<UVFF>(destMthd, callSite, zerovalue);
  }
  return Identity<IFDSUnitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getRetFlowFunction(
    IFDSUnitializedVariables::n_t callSite,
    IFDSUnitializedVariables::m_t calleeMthd,
    IFDSUnitializedVariables::n_t exitStmt,
    IFDSUnitializedVariables::n_t retSite) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getRetFlowFunction()");
  if (llvm::isa<llvm::CallInst>(callSite) ||
      llvm::isa<llvm::InvokeInst>(callSite)) {
    llvm::ImmutableCallSite CS(callSite);
    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      llvm::ImmutableCallSite call;
      const llvm::Instruction *exit;
      UVFF(llvm::ImmutableCallSite c, const llvm::Instruction *e)
          : call(c), exit(e) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        // check if we return an uninitialized value
        if (exit->getNumOperands() > 0 && exit->getOperand(0) == source) {
          return {call.getInstruction()};
        }
        // kill all other facts
        return {};
      }
    };
    return make_shared<UVFF>(CS, exitStmt);
  }
  // kill everything else
  return KillAll<IFDSUnitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getCallToRetFlowFunction(
    IFDSUnitializedVariables::n_t callSite,
    IFDSUnitializedVariables::n_t retSite,
    set<IFDSUnitializedVariables::m_t> callees) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getCallToRetFlowFunction()");
  return Identity<IFDSUnitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getSummaryFlowFunction(
    IFDSUnitializedVariables::n_t callStmt,
    IFDSUnitializedVariables::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getSummaryFlowFunction()");
  return nullptr;
}

map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::initialSeeds()");
  map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                  set<IFDSUnitializedVariables::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSUnitializedVariables::d_t IFDSUnitializedVariables::createZeroValue() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSUnitializedVariables::isZeroValue(
    IFDSUnitializedVariables::d_t d) const {
  return isLLVMZeroValue(d);
}

void IFDSUnitializedVariables::printNode(
    ostream &os, IFDSUnitializedVariables::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSUnitializedVariables::printDataFlowFact(
    ostream &os, IFDSUnitializedVariables::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSUnitializedVariables::printMethod(
    ostream &os, IFDSUnitializedVariables::m_t m) const {
  os << m->getName().str();
}

void IFDSUnitializedVariables::printIFDSReport(
    ostream &os,
    SolverResults<IFDSUnitializedVariables::n_t, IFDSUnitializedVariables::d_t,
                  BinaryDomain> &SR) {
  os << "=========== IFDS Uninitialized Analysis Results ===========\n";
  if (UndefValueUses.empty()) {
    os << "No uninitialized variables were used!\n";
  } else {
    for (auto User : UndefValueUses) {
      os << "At instruction\nIR  : ";
      printNode(os, User.first);
      os << '\n';
      // os << llvmValueToSrc(User.first)
      //  << "\n\nUsed uninitialized variable(s):\n";
      for (auto UndefV : User.second) {
        os << "IR  : ";
        printDataFlowFact(os, UndefV);
        os << '\n';
        //  os << llvmValueToSrc(UndefV) << '\n';
      }
      os << "-----------------------------------------------------------\n\n";
    }
  }
}

} // namespace psr
