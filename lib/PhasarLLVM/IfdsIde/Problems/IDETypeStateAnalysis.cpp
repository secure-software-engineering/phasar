/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETypeStateAnalysis.h>

#include <phasar/Utils/LLVMShorthands.h>
//#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

const State IDETypeStateAnalysis::TOP = uninit;

const State IDETypeStateAnalysis::BOTTOM = error;

IDETypeStateAnalysis::IDETypeStateAnalysis(IDETypeStateAnalysis::i_t icfg,
                                           vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t curr,
                                            IDETypeStateAnalysis::n_t succ) {
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Alloca,
                                                             zeroValue());
        }
      }
    }
    // struct UnsereFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
    //   const llvm::AllocaInst *Alloc;
    //   const llvm::Value *ZV;
    //   UnsereFlowFunction(const llvm::AllocaInst *A, const llvm::Value *Z)
    //       : Alloc(A), ZV(Z) {}

    //   set<IDETypeStateAnalysis::d_t>
    //   computeTargets(IDETypeStateAnalysis::d_t source) override {
    //     if (source == ZV) {
    //       return {source, Alloc};
    //     } else {
    //       return {source};
    //     }
    //   }
    // };
    // return make_shared<UnsereFlowFunction>(Alloca, zeroValue());
    // return make_shared<Lambda<IDETypeStateAnalysis::d_t>>([Variablen der
    // "Außenwelt können hier gecaptured werden"](IDETypeStateAnalysis::d_t
    // source) {

    //   return set<IDETypeStateAnalysis::d_t>{};
    // });
  }

  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    /**/if (Load->getPointerOperand()->getType()->getPointerElementType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Load->getPointerOperand()->getType()->getPointerElementType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Load,
                                                             zeroValue());
        }
      }
    /**/}
  }

  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Store->getValueOperand()->getType()->getPointerElementType())) {
        if(StructTy->getName().find("struct._IO_FILE") != 
            llvm::StringRef::npos){
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Store,
                                                             zeroValue());
        }
      }
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t callStmt,
                                          IDETypeStateAnalysis::m_t destMthd) {
  /*auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IDETypeStateAnalysis::getCallFlowFunction()");*/

  if (destMthd->getName() == "fopen" || destMthd->getName() == "freopen" || destMthd->getName() == "fgetc" || destMthd->getName() == "fputc" || destMthd->getName() == "putchar" ||
      destMthd->getName() == "_IO_getc" || destMthd->getName() == "_I0_putc" || destMthd->getName() == "fprintf" || destMthd->getName() == "__isoc99_fscanf" || destMthd->getName() == "feof" ||
      destMthd->getName() == "ferror" || destMthd->getName() == "ungetc" || destMthd->getName() == "fflush" || destMthd->getName() == "fseek" || destMthd->getName() == "ftell" || 
      destMthd->getName() == "rewind" || destMthd->getName() == "fgetpos" || destMthd->getName() == "fsetpos") {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }

  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destMthd);
  }

  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getRetFlowFunction(IDETypeStateAnalysis::n_t callSite,
                                         IDETypeStateAnalysis::m_t calleeMthd,
                                         IDETypeStateAnalysis::n_t exitStmt,
                                         IDETypeStateAnalysis::n_t retSite) {
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](IDETypeStateAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      });
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::n_t retSite,
    set<IDETypeStateAnalysis::m_t> callees) {
  for (auto Callee : callees) {

    if (Callee->getName() == "fopen") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "freopen") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "fgetc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }
    
    if(Callee->getName() == "fputc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "putchar") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "_IO_getc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "_I0_putc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fprintf") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "__isoc99_fscanf") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "feof") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "ferror") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "ungetc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "fflush") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "fseek") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "ftell") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }  

    if(Callee->getName() == "rewind") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "fgetpos") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if(Callee->getName() == "fsetpos") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getSummaryFlowFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::m_t destMthd) {
  return nullptr;
}

map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::initialSeeds() {
  // just start in main()
  map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                       set<IDETypeStateAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDETypeStateAnalysis::d_t IDETypeStateAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETypeStateAnalysis::isZeroValue(IDETypeStateAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t curr, IDETypeStateAnalysis::d_t currNode,
    IDETypeStateAnalysis::n_t succ, IDETypeStateAnalysis::d_t succNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getCallEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t srcNode,
    IDETypeStateAnalysis::m_t destiantionMethod,
    IDETypeStateAnalysis::d_t destNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::m_t calleeMethod,
    IDETypeStateAnalysis::n_t exitStmt, IDETypeStateAnalysis::d_t exitNode,
    IDETypeStateAnalysis::n_t reSite, IDETypeStateAnalysis::d_t retNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getCallToReturnEdgeFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::v_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::v_t lhs,
                           IDETypeStateAnalysis::v_t rhs) {
  return (lhs == BOTTOM || rhs == BOTTOM) ? BOTTOM : TOP;
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::v_t>>(TOP);
}

string IDETypeStateAnalysis::DtoString(IDETypeStateAnalysis::d_t d) const {
  return llvmIRToString(d);
}

string IDETypeStateAnalysis::VtoString(IDETypeStateAnalysis::v_t v) const {
  return to_string(static_cast<int>(v));
}

string IDETypeStateAnalysis::NtoString(IDETypeStateAnalysis::n_t n) const {
  return llvmIRToString(n);
}

string IDETypeStateAnalysis::MtoString(IDETypeStateAnalysis::m_t m) const {
  return m->getName().str();
}

} // namespace psr
