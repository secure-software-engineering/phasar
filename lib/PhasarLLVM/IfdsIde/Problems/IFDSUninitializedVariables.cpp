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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/LambdaFlow.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>

#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

IFDSUninitializedVariables::IFDSUninitializedVariables(
    IFDSUninitializedVariables::i_t icfg, const LLVMTypeHierarchy &th,
    const ProjectIRDB &irdb, vector<string> EntryPoints)
    : LLVMDefaultIFDSTabulationProblem(icfg, th, irdb),
      EntryPoints(EntryPoints) {
  IFDSUninitializedVariables::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::getNormalFlowFunction(
    IFDSUninitializedVariables::n_t curr,
    IFDSUninitializedVariables::n_t succ) {
  //----------------------------------------------------------------------------------
  // Why do we need this case?
  // Every alloca is reached eventually by this function
  //----------------------------------------------------------------------------------

  // initially mark every local as uninitialized (except entry point args)
  /* if (icfg.isStartPoint(curr) &&
            curr->getFunction()->getName().str() == "main") {
    const llvm::Function *func = icfg.getMethodOf(curr);
    // set all locals as uninitialized flow function
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      const llvm::Function *func;
      IFDSUninitializedVariables::d_t zerovalue;
      UVFF(const llvm::Function *f, IFDSUninitializedVariables::d_t zv)
          : func(f), zerovalue(zv) {}
      set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t source) override {
        if (source == zerovalue) {
          set<IFDSUninitializedVariables::d_t> res;

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
  */

  // check the all store instructions and kill initialized variables
  if (auto store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      // const llvm::Value *valueop;
      // const llvm::Value *pointerop;
      const llvm::StoreInst *store;
      const llvm::Value *zero;
      map<IFDSUninitializedVariables::n_t, set<IFDSUninitializedVariables::d_t>>
          &UndefValueUses;
      UVFF(const llvm::StoreInst *s,
           map<IFDSUninitializedVariables::n_t,
               set<IFDSUninitializedVariables::d_t>> &UVU,
           const llvm::Value *zero)
          : store(s), zero(zero), UndefValueUses(UVU) {}
      set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t source) override {

        //----------------------------------------------------------------------
        // I don't get the purpose of this for-loop;
        // When an undefined load is stored, it should eventually be source
        //----------------------------------------------------------------------

        /*
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
         */
        if (source == store->getValueOperand() ||
            (source == zero &&
             llvm::isa<llvm::UndefValue>(store->getValueOperand())))
          return {source, store->getPointerOperand()};
        else if (source ==
                 store->getPointerOperand()) // storing an initialized value
                                             // kills the variable as it is
                                             // now initialized too
          return {};
        // pass all other facts as identity
        return {source};
      }
    };
    return make_shared<UVFF>(store, UndefValueUses, zerovalue);
  }
  if (auto alloc = llvm::dyn_cast<llvm::AllocaInst>(curr)) {

    return make_shared<LambdaFlow<IFDSUninitializedVariables::d_t>>(
        [alloc, this](IFDSUninitializedVariables::d_t source)
            -> set<IFDSUninitializedVariables::d_t> {
          if (isZeroValue(source)) {
            if (alloc->getAllocatedType()->isIntegerTy() ||
                alloc->getAllocatedType()->isFloatingPointTy() ||
                alloc->getAllocatedType()->isPointerTy() ||
                alloc->getAllocatedType()->isArrayTy()) {
              //------------------------------------------------------------
              // Why not generate for structs, but for arrays? (would be
              // consistent to generate either both or none of them)
              //------------------------------------------------------------

              // generate the alloca
              return {source, alloc};
            }
          }
          // otherwise propagate all facts
          return {source};
        });
  }
  // check if some instruction is using an undefined value (in)directly
  struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
    const llvm::Instruction *inst;
    map<IFDSUninitializedVariables::n_t, set<IFDSUninitializedVariables::d_t>>
        &UndefValueUses;
    UVFF(const llvm::Instruction *inst,
         map<IFDSUninitializedVariables::n_t,
             set<IFDSUninitializedVariables::d_t>> &UVU)
        : inst(inst), UndefValueUses(UVU) {}
    set<IFDSUninitializedVariables::d_t>
    computeTargets(IFDSUninitializedVariables::d_t source) override {
      for (auto &operand : inst->operands()) {
        const llvm::UndefValue *undef =
            llvm::dyn_cast<llvm::UndefValue>(operand);
        if (operand == source || operand == undef) {
          //----------------------------------------------------------------
          // It is not necessary and (from my point of view) not intended to
          // report a leak on EVERY kind of instruction.
          // For some of them (e.g. gep, bitcast, ...) propagating the dataflow
          // facts may be enough
          //----------------------------------------------------------------
          if (!llvm::isa<llvm::GetElementPtrInst>(inst) &&
              !llvm::isa<llvm::CastInst>(inst) &&
              !llvm::isa<llvm::PHINode>(inst))
            UndefValueUses[inst].insert(operand);
          return {source, inst};
        }
      }
      return {source};
    }
  };
  return make_shared<UVFF>(curr, UndefValueUses);

  // otherwise we do not care and nothing changes
  return Identity<IFDSUninitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::getCallFlowFunction(
    IFDSUninitializedVariables::n_t callStmt,
    IFDSUninitializedVariables::m_t destMthd) {
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    llvm::ImmutableCallSite callSite(callStmt);
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
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
        /*for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
          formals.push_back(getNthFunctionArgument(destMthd, idx));
        }*/
        for (auto &arg : destMthd->args()) {
          formals.push_back(&arg);
        }
      }

      set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t source) override {
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

          //--------------------------------------------------------------
          // Why not letting the normal FF generate the allocas?
          //--------------------------------------------------------------

          // on zerovalue -> gen all locals parameter
          /* set<const llvm::Value *> res;
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
          return res;*/
          return {};
        }
      }
    };
    return make_shared<UVFF>(destMthd, callSite, zerovalue);
  }
  return Identity<IFDSUninitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::getRetFlowFunction(
    IFDSUninitializedVariables::n_t callSite,
    IFDSUninitializedVariables::m_t calleeMthd,
    IFDSUninitializedVariables::n_t exitStmt,
    IFDSUninitializedVariables::n_t retSite) {
  if (llvm::isa<llvm::CallInst>(callSite) ||
      llvm::isa<llvm::InvokeInst>(callSite)) {
    llvm::ImmutableCallSite CS(callSite);
    struct UVFF : FlowFunction<IFDSUninitializedVariables::d_t> {
      llvm::ImmutableCallSite call;
      const llvm::Instruction *exit;
      UVFF(llvm::ImmutableCallSite c, const llvm::Instruction *e)
          : call(c), exit(e) {}
      set<IFDSUninitializedVariables::d_t>
      computeTargets(IFDSUninitializedVariables::d_t source) override {
        // check if we return an uninitialized value
        set<IFDSUninitializedVariables::d_t> ret;
        if (exit->getNumOperands() > 0 && exit->getOperand(0) == source) {
          ret.insert(call.getInstruction());
        }
        //----------------------------------------------------------------------
        // Handle pointer/reference parameters
        //----------------------------------------------------------------------
        if (call.getCalledFunction()) {
          unsigned i = 0;
          for (auto &arg : call.getCalledFunction()->args()) {
            // auto arg = getNthFunctionArgument(call.getCalledFunction(), i);
            if (&arg == source && arg.getType()->isPointerTy()) {
              ret.insert(call.getArgument(i));
            }
            i++;
          }
        }

        // kill all other facts
        return ret;
      }
    };
    return make_shared<UVFF>(CS, exitStmt);
  }
  // kill everything else
  return KillAll<IFDSUninitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::getCallToRetFlowFunction(
    IFDSUninitializedVariables::n_t callSite,
    IFDSUninitializedVariables::n_t retSite,
    set<IFDSUninitializedVariables::m_t> callees) {
  //----------------------------------------------------------------------
  // Handle pointer/reference parameters
  //----------------------------------------------------------------------
  if (llvm::isa<llvm::CallInst>(callSite) ||
      llvm::isa<llvm::InvokeInst>(callSite)) {
    llvm::ImmutableCallSite CS(callSite);
    return make_shared<LambdaFlow<IFDSUninitializedVariables::d_t>>(
        [CS](IFDSUninitializedVariables::d_t source)
            -> set<IFDSUninitializedVariables::d_t> {
          if (source->getType()->isPointerTy()) {
            for (auto &arg : CS.args()) {
              if (arg.get() == source)
                // do not propagate pointer arguments, since the function may
                // initialize them (would be much more precise with
                // field-sensitivity)
                return {};
            }
          }
          return {source};
        });
  }
  return Identity<IFDSUninitializedVariables::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::getSummaryFlowFunction(
    IFDSUninitializedVariables::n_t callStmt,
    IFDSUninitializedVariables::m_t destMthd) {
  return nullptr;
}

map<IFDSUninitializedVariables::n_t, set<IFDSUninitializedVariables::d_t>>
IFDSUninitializedVariables::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUninitializedVariables::initialSeeds()");
  map<IFDSUninitializedVariables::n_t, set<IFDSUninitializedVariables::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                  set<IFDSUninitializedVariables::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSUninitializedVariables::d_t IFDSUninitializedVariables::createZeroValue() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSUninitializedVariables::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSUninitializedVariables::isZeroValue(
    IFDSUninitializedVariables::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSUninitializedVariables::printNode(
    ostream &os, IFDSUninitializedVariables::n_t n) const {
  os << llvmIRToShortString(n);
}

void IFDSUninitializedVariables::printDataFlowFact(
    ostream &os, IFDSUninitializedVariables::d_t d) const {
  os << llvmIRToShortString(d);
}

void IFDSUninitializedVariables::printMethod(
    ostream &os, IFDSUninitializedVariables::m_t m) const {
  os << m->getName().str();
}

void IFDSUninitializedVariables::printIFDSReport(
    ostream &os,
    SolverResults<IFDSUninitializedVariables::n_t,
                  IFDSUninitializedVariables::d_t, BinaryDomain> &SR) {
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
const std::map<IFDSUninitializedVariables::n_t,
               std::set<IFDSUninitializedVariables::d_t>> &
IFDSUninitializedVariables::getAllUndefUses() const {
  return UndefValueUses;
}

} // namespace psr
