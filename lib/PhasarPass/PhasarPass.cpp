/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarPass/Options.h>
#include <phasar/PhasarPass/PhasarPass.h>
#include <phasar/Utils/EnumFlags.h>

namespace psr {

char PhasarPass::ID = 12;

PhasarPass::PhasarPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPass::getPassName() const { return "PhasarPass"; }

bool PhasarPass::runOnModule(llvm::Module &M) {
  IRDBOptions Opt = IRDBOptions::NONE;
  Opt |= IRDBOptions::WPA;
  Opt |= IRDBOptions::OWNSNOT;

  ProjectIRDB DB(Opt);
  DB.insertModule(std::unique_ptr<llvm::Module>(&M));
  DB.preprocessIR();
  if (DB.getFunction("main")) {
    LLVMTypeHierarchy H(DB);
    LLVMBasedICFG I(H, DB, CallGraphAnalysisType::OTF, {"main"});
    llvm::outs() << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    // IFDS template parametrization test
    IFDSLinearConstantAnalysis L(I, {"main"});
    LLVMIFDSSolver<LCAPair, LLVMBasedICFG &> S(L, true);
    S.solve();
    // IDE template parametrization test
    IDELinearConstantAnalysis M(I, {"main"});
    LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> T(M, true);
    T.solve();
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return false;
}

bool PhasarPass::doInitialization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doInitialization()\n";
  return false;
}

bool PhasarPass::doFinalization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doFinalization()\n";
  return false;
}

void PhasarPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

void PhasarPass::releaseMemory() {}

void PhasarPass::print(llvm::raw_ostream &O, const llvm::Module *M) const {
  O << "I am a PhasarPass Analysis Result ;-)\n";
}

static llvm::RegisterPass<PhasarPass> phasar("phasar", "PhASAR Pass",
                                             false /* Only looks at CFG */,
                                             false /* Analysis Pass */);

} // namespace psr
