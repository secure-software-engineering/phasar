#include <iostream>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "MyIFDSProblem.h"

using namespace std;
using namespace psr;

// Factory function that is used to create an instance by the Phasar framework.
unique_ptr<IFDSTabulationProblemPlugin>
makeMyIFDSProblem(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                  const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                  std::set<std::string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new MyIFDSProblem(IRDB, TH, ICF, PT, EntryPoints));
}

// Is executed on plug-in load and has to register this plug-in to Phasar.
__attribute__((constructor)) void init() {
  cout << "init - MyIFDSProblem\n";
  IFDSTabulationProblemPluginFactory["MyIFDSProblem"] = &makeMyIFDSProblem;
}

// Is executed on unload, can be used to unregister the plug-in.
__attribute__((destructor)) void fini() { cout << "fini - MyIFDSProblem\n"; }

MyIFDSProblem::MyIFDSProblem(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
    : IFDSTabulationProblemPlugin(IRDB, TH, ICF, PT, EntryPoints) {
  ZeroValue = FFManager.getOrCreateZero();
}

 const FlowFact *MyIFDSProblem::createZeroValue() const {
   return ZeroValue;
 }

MyIFDSProblem::FlowFunctionPtrType
MyIFDSProblem::getNormalFlowFunction(const llvm::Instruction *curr,
                                     const llvm::Instruction *succ) {
  cout << "MyIFDSProblem::getNormalFlowFunction()\n";
  // TODO: Must be implemented to propagate tainted values through the program.
  // Tainted values may spread through loads and stores (llvm::LoadInst and
  // llvm::StoreInst). Important memberfunctions are getPointerOperand() and
  // getPointerOperand()/ getValueOperand() respectively.
  return Identity<const FlowFact *>::getInstance();
}

MyIFDSProblem::FlowFunctionPtrType
MyIFDSProblem::getCallFlowFunction(const llvm::Instruction *callStmt,
                                   const llvm::Function *destMthd) {
  cout << "MyIFDSProblem::getCallFlowFunction()\n";
  // TODO: Must be modeled to perform parameter passing:
  // actuals at caller-side must be mapped into formals at callee-side.
  // LLVM distinguishes between a ordinary function call and a function call
  // that might throw (llvm::CallInst, llvm::InvokeInst). To be able to easily
  // inspect both, a variable of type llvm::ImmutableCallSite may be
  // constructed using 'callStmt'.
  // Important: getCallFlowFunction() can also be used in combination with
  // getCallToRetFlowFunction() in order to model a function's effect without
  // actually following call targets. This must be used to model sources and
  // sinks of the taint analysis. It works by killing all flow facts at the
  // call-site and generating the desired facts within the
  // getCallToRetFlowFunction.
  return Identity<const FlowFact *>::getInstance();
}

MyIFDSProblem::FlowFunctionPtrType MyIFDSProblem::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "MyIFDSProblem::getRetFlowFunction()\n";
  // TODO: Must be modeled to map the return value back into the caller's
  // context. When dealing with pointer parameters one must also map the
  // formals at callee-side back into the actuals at caller-side. All other
  // facts that do not influence the caller must be killed.
  // 'callSite' can be handled by using llvm::ImmutableCallSite, 'exitStmt' is
  // the function's return instruction - llvm::ReturnInst may be used.
  // The 'retSite' is - in case of LLVM - the call-site and it is possible
  // to wrap it into an llvm::ImmutableCallSite.
  return Identity<const FlowFact *>::getInstance();
}

MyIFDSProblem::FlowFunctionPtrType
MyIFDSProblem::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                        const llvm::Instruction *retSite,
                                        set<const llvm::Function *> callees) {
  cout << "MyIFDSProblem::getCallToRetFlowFunction()\n";
  // TODO: Use in combination with getCallFlowFunction to model the effects of
  // source and sink functions. It most analyses flow facts can be passed as
  // identity.
  return Identity<const FlowFact *>::getInstance();
}

// May be used to model function calls to libc or llvm.intrinsic functions
// for which no implementation is accessible. If nullptr is returned it applies
// identity on all flow facts that are present.
MyIFDSProblem::FlowFunctionPtrType
MyIFDSProblem::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  cout << "MyIFDSProblem::getSummaryFlowFunction()\n";
  return nullptr;
}

// Return(s) set(s) of flow fact(s) that hold(s) initially at a corresponding
// statement. The analysis will start at these instructions and propagate the
// flow facts according to the analysis description.
map<const llvm::Instruction *, set<const FlowFact *>>
MyIFDSProblem::initialSeeds() {
  cout << "MyIFDSProblem::initialSeeds()\n";
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  auto EntryPoints = getEntryPoints();
  for (const auto &EntryPoint : EntryPoints) {
    if (const auto *F = IRDB->getFunctionDefinition(EntryPoint)) {
      SeedMap.insert(std::make_pair(&F->front().front(),
                                    set<const FlowFact *>({getZeroValue()})));
    } else {
      cout << "Could not retrieve function '" << EntryPoint
           << "' --> Function does not exist or is declaration only.\n";
    }
  }
  return SeedMap;
}
