/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultSeeds.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
using namespace std;

IFDSUnitializedVariables::IFDSUnitializedVariables(
    IFDSUnitializedVariables::i_t icfg, vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSUnitializedVariables::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getNormalFlowFunction(
    IFDSUnitializedVariables::n_t curr, IFDSUnitializedVariables::n_t succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IFDSUnitializedVariables::getNormalFlowFunction()";
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
      computeTargets(IFDSUnitializedVariables::d_t source) {
        if (source == zerovalue) {
          set<IFDSUnitializedVariables::d_t> res;
          // first add all local values of primitive types
          for (auto &BB : *func) {
            for (auto &inst : BB) {
              if (const llvm::AllocaInst *alloc =
                      llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
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
              if (const llvm::StoreInst *store =
                      llvm::dyn_cast<llvm::StoreInst>(user)) {
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
  if (const llvm::StoreInst *store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    const llvm::Value *valueop = store->getValueOperand();
    const llvm::Value *pointerop = store->getPointerOperand();

    struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
      const llvm::Value *valueop;
      const llvm::Value *pointerop;
      UVFF(const llvm::Value *vop, const llvm::Value *pop)
          : valueop(vop), pointerop(pop) {}
      set<IFDSUnitializedVariables::d_t>
      computeTargets(IFDSUnitializedVariables::d_t source) {
        // check if an uninitialized value is loaded and stored in a variable,
        // then the variable is uninitialized!
        for (auto &use : valueop->uses()) {
          if (const llvm::LoadInst *load =
                  llvm::dyn_cast<llvm::LoadInst>(use)) {
            // if the following is uninit, then this store must be uninit as
            // well!
            if (load->getPointerOperand() == source) {
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
    return make_shared<UVFF>(valueop, pointerop);
  }

  // check if some instruction is using an undefined value directly or
  // indirectly
  struct UVFF : FlowFunction<IFDSUnitializedVariables::d_t> {
    const llvm::Instruction *inst;
    UVFF(const llvm::Instruction *inst) : inst(inst) {}
    set<IFDSUnitializedVariables::d_t>
    computeTargets(IFDSUnitializedVariables::d_t source) {
      for (auto &operand : inst->operands()) {
        if (operand == source) {
          return {source, inst};
        }
      }
      for (auto &operand : inst->operands()) {
        if (const llvm::UndefValue *undef =
                llvm::dyn_cast<llvm::UndefValue>(operand)) {
          return {source, inst};
        }
      }
      return {source};
    }
  };
  return make_shared<UVFF>(curr);

  // otherwise we do not care and nothing changes
  return Identity<IFDSUnitializedVariables::d_t>::v();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getCallFlowFunction(
    IFDSUnitializedVariables::n_t callStmt,
    IFDSUnitializedVariables::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSUnitializedVariables::getCallFlowFunction()";
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
              if (const llvm::AllocaInst *alloc =
                      llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
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
  } else if (const llvm::InvokeInst *invoke =
                 llvm::dyn_cast<llvm::InvokeInst>(callStmt)) {
    /*
     * TODO consider an invoke statement
     * An invoke statement must be treated the same as an ordinary call
     * statement
     */
    return Identity<IFDSUnitializedVariables::d_t>::v();
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
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSUnitializedVariables::getRetFlowFunction()";
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
      computeTargets(IFDSUnitializedVariables::d_t source) {
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
  return KillAll<IFDSUnitializedVariables::d_t>::v();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getCallToRetFlowFunction(
    IFDSUnitializedVariables::n_t callSite,
    IFDSUnitializedVariables::n_t retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IFDSUnitializedVariables::getCallToRetFlowFunction()";
  // handle a normal use of an initialized return value
  for (auto user : callSite->users()) {
    return make_shared<Kill<IFDSUnitializedVariables::d_t>>(user);
  }
  return Identity<IFDSUnitializedVariables::d_t>::v();
}

shared_ptr<FlowFunction<IFDSUnitializedVariables::d_t>>
IFDSUnitializedVariables::getSummaryFlowFunction(
    IFDSUnitializedVariables::n_t callStmt,
    IFDSUnitializedVariables::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG)
      << "IFDSUnitializedVariables::getSummaryFlowFunction()";
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
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSUnitializedVariables::initialSeeds()";
  map<IFDSUnitializedVariables::n_t, set<IFDSUnitializedVariables::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                       set<IFDSUnitializedVariables::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSUnitializedVariables::d_t IFDSUnitializedVariables::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSUnitializedVariables::createZeroValue()";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSUnitializedVariables::isZeroValue(
    IFDSUnitializedVariables::d_t d) const {
  return isLLVMZeroValue(d);
}

string
IFDSUnitializedVariables::DtoString(IFDSUnitializedVariables::d_t d) const {
  return llvmIRToString(d);
}

string
IFDSUnitializedVariables::NtoString(IFDSUnitializedVariables::n_t n) const {
  return llvmIRToString(n);
}

string
IFDSUnitializedVariables::MtoString(IFDSUnitializedVariables::m_t m) const {
  return m->getName().str();
}
