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
  // set every local variable as uninitialized, that is not a function parameter
  if (curr->getFunction()->getName().str() == "main" &&
      icfg.isStartPoint(curr)) {
    curr->print(llvm::outs());
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
              if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                auto alloc_type = alloc->getAllocatedType();
                if (alloc_type->isIntegerTy() ||
                    alloc_type->isFloatingPointTy() ||
                    alloc_type->isPointerTy() || alloc_type->isArrayTy()) {
                  res.insert(alloc);
                }
              } else {
                // when the very first instruction immediately uses an undef
                // value
                for (auto &operand : inst.operands()) {
                  if (const llvm::UndefValue *undef =
                          llvm::dyn_cast<llvm::UndefValue>(&operand)) {
                    res.insert(&inst);
                  }
                }
              }
            }
          }
          // now remove those values that are obtained by function parameters of
          // the entry function
          for (auto &arg : func->args()) {
            for (auto user : arg.users()) {
              if (auto store = llvm::dyn_cast<llvm::StoreInst>(user)) {
                res.erase(store->getPointerOperand());
              }
            }
          }
          res.insert(zerovalue);
          return res;
        }
        return set<IFDSUnitializedVariables::d_t>{};
      }
    };
    return make_shared<UVFF>(func, zerovalue);
  }

  // check the all store instructions
  if (auto *store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    const llvm::Value *valueop = store->getValueOperand();
    const llvm::Value *pointerop = store->getPointerOperand();

    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Value *valueop;
      const llvm::Value *pointerop;
      map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
          &UndefValueUses;
      UVFF(const llvm::Value *vop, const llvm::Value *pop,
           map<IFDSUnitializedVariables::n_t,
               set<IFDSUnitializedVariables::d_t>> &UVU)
          : valueop(vop), pointerop(pop), UndefValueUses(UVU) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        // check if an uninitialized value is loaded and stored in a variable,
        // then the variable is uninitialized!
        for (auto &use : valueop->uses()) {
          if (const llvm::LoadInst *load =
                  llvm::dyn_cast<llvm::LoadInst>(use)) {
            auto LoadPointerOp = load->getPointerOperand();
            // if the following is uninit, then this store must be uninit as
            // well!
            if (source == LoadPointerOp) {
              UndefValueUses[load].insert(LoadPointerOp);
              return {source, pointerop};
            }
          }
          if (const llvm::Instruction *inst =
                  llvm::dyn_cast<llvm::Instruction>(use)) {
            for (auto &operand : inst->operands()) {
              if (operand == source) {
                return {source, pointerop};
              }
            }
            for (auto &operand : inst->operands()) {
              if (const llvm::UndefValue *undef =
                      llvm::dyn_cast<llvm::UndefValue>(operand)) {
                return {source, pointerop};
              }
            }
          }
        }
        // otherwise the value is initialized through this store and thus can be
        // killed
        if (pointerop == source) {
          return {};
        } else {
          return {source};
        }
      }
    };
    return make_shared<UVFF>(valueop, pointerop, UndefValueUses);
  }

  // check if some instruction is using an undefined value directly or
  // indirectly
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
        if (operand == source || undef) {
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
  // check for a usual function call
  if (const llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(callStmt)) {
    // if (call->getCalledFunction()) {
    //   cout << "DIRECT CALL TO: " << destMthd->getName().str() << endl;
    // } else {
    //   cout << "INDIRECT CALL TO: " << destMthd->getName().str() << endl;
    // }

    // collect the actual parameters
    vector<const llvm::Value *> actuals;
    for (auto &operand : call->operands()) {
      actuals.push_back(operand);
    }

    // cout << "ACTUALS:" << endl;
    // for (auto a : actuals) {
    //   if (a)
    //     a->print(llvm::outs());
    // }

    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Function *destMthd;
      const llvm::CallInst *call;
      vector<const llvm::Value *> actuals;
      const llvm::Value *zerovalue;
      UVFF(const llvm::Function *dm, const llvm::CallInst *c,
           vector<const llvm::Value *> atl, const llvm::Value *zv)
          : destMthd(dm), call(c), actuals(atl), zerovalue(zv) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        // do the mapping from actual to formal parameters
        for (size_t i = 0; i < actuals.size(); ++i) {
          if (actuals[i] == source) {
            // cout << "ACTUAL == SOURCE" << endl;
            return {call->getOperand(i)};
          }
          //      		if (const llvm::UndefValue* undef =
          //      llvm::dyn_cast<llvm::UndefValue>(actuals[i])) {
          //      			return { undef };
          //      		}
        }

        if (source == zerovalue) {
          // gen all locals that are not parameter locals!!!
          // make a set of all uninitialized local variables!
          set<const llvm::Value *> uninitlocals;
          for (auto &BB : *destMthd) {
            for (auto &inst : BB) {
              if (auto *alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                // check if the allocated value is of a primitive type
                auto alloc_type = alloc->getAllocatedType();
                if (alloc_type->isIntegerTy() ||
                    alloc_type->isFloatingPointTy() ||
                    alloc_type->isPointerTy() || alloc_type->isArrayTy()) {
                  uninitlocals.insert(alloc);
                }
              } else {
                for (auto &operand : inst.operands()) {
                  if (const llvm::UndefValue *undef =
                          llvm::dyn_cast<llvm::UndefValue>(&operand)) {
                    uninitlocals.insert(operand);
                  }
                }
                for (auto &operand : call->operands()) {
                  if (const llvm::UndefValue *undef =
                          llvm::dyn_cast<llvm::UndefValue>(&operand)) {
                    uninitlocals.insert(operand);
                  }
                }
              }
            }
          }
          // remove all local variables, that are initialized formal parameters!
          for (auto &arg : destMthd->args()) {
            uninitlocals.erase(&arg);
          }
          return uninitlocals;
        }
        return set<const llvm::Value *>{};
      }
    };
    return make_shared<UVFF>(destMthd, call, actuals, zerovalue);
  } else if (auto invoke = llvm::dyn_cast<llvm::InvokeInst>(callStmt)) {
    /*
     * TODO consider an invoke statement
     * An invoke statement must be treated the same as an ordinary call
     * statement
     */
    return Identity<IFDSUnitializedVariables::d_t>::getInstance();
  }
  cout << "error when getCallFlowFunction() was called\n"
          "instruction is neither a call- nor an invoke instruction!"
       << endl;
  DIE_HARD;
  return nullptr;
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
  // consider it a value gets store at the call site:
  // int x = call(...);
  // x shall be uninitialized then
  // check if callSite is usual call instruction
  if (const llvm::ReturnInst *ret =
          llvm::dyn_cast<llvm::ReturnInst>(exitStmt)) {
    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::CallInst *call;
      const llvm::ReturnInst *ret;
      UVFF(const llvm::CallInst *c, const llvm::ReturnInst *r)
          : call(c), ret(r) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) override {
        if (ret->getNumOperands() > 0 && ret->getOperand(0) == source) {
          set<const llvm::Value *> results;
          // users of this call instruction get an uninitialized value!
          for (auto user : call->users()) {
            results.insert(user);
          }
          if (results.empty()) {
            results.insert(call);
          }
          return results;
        }
        return {};
      }
    };
    if (const llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(callSite)) {
      return make_shared<UVFF>(call, ret);
    }
  }
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
  // handle a normal use of an initialized return value
  for (auto user : callSite->users()) {
    return make_shared<Kill<IFDSUnitializedVariables::d_t>>(user);
  }
  return Identity<IFDSUnitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getSummaryFlowFunction(
    IFDSUnitializedVariables::n_t callStmt,
    IFDSUnitializedVariables::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUnitializedVariables::getSummaryFlowFunction()");
  SpecialSummaries<IFDSUnitializedVariables::d_t, BinaryDomain> &SpecialSum =
      SpecialSummaries<IFDSUnitializedVariables::d_t,
                       BinaryDomain>::getInstance();
  if (SpecialSum.containsSpecialSummary(destMthd)) {
    // return SpecialSum.getSpecialFlowFunctionSummary(destMthd);
    return nullptr;
  } else {
    return nullptr;
  }
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
      os << "At instruction\nIR  : " << NtoString(User.first) << '\n'
         << llvmValueToSrc(User.first)
         << "\n\nUsed uninitialized variable(s):\n";
      for (auto UndefV : User.second) {
        os << "IR  : " << DtoString(UndefV) << '\n'
           << llvmValueToSrc(UndefV) << '\n';
      }
      os << "-----------------------------------------------------------\n\n";
    }
  }
}

} // namespace psr
