#include <iostream>
#include <utility>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"

#include "IDETabulationProblemTestPlugin.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IDETabulationProblemPlugin> makeIDETabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints) {
  return unique_ptr<IDETabulationProblemPlugin>(
      new IDETabulationProblemTestPlugin(IRDB, TH, ICF, PT,
                                         std::move(EntryPoints)));
}

__attribute__((constructor)) void init() {
  cout << "init - IDETabulationProblemTestPlugin\n";
  IDETabulationProblemPluginFactory["ide_testplugin"] =
      &makeIDETabulationProblemTestPlugin;
  cout << "init done" << std::endl;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IDETabulationProblemTestPlugin\n";
}

IDETabulationProblemTestPlugin::IDETabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *Curr, const llvm::Instruction *Succ) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function *CalleeFun,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    set<const llvm::Function *> Callees) {
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDETabulationProblemTestPlugin::initialSeeds() {
  cout << "IDETabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                       set<const llvm::Value *>({getZeroValue()})));
  }
  return SeedMap;
}

std::shared_ptr<EdgeFunction<IDETabulationProblemTestPlugin::l_t>>
IDETabulationProblemTestPlugin::getNormalEdgeFunction(n_t curr, d_t currNode,
                                                      n_t succ, d_t succNode) {
  return EdgeIdentity<l_t>::getInstance();
}
std::shared_ptr<EdgeFunction<IDETabulationProblemTestPlugin::l_t>>
IDETabulationProblemTestPlugin::getCallEdgeFunction(n_t callStmt, d_t srcNode,
                                                    f_t destinationFunction,
                                                    d_t destNode) {
  return EdgeIdentity<l_t>::getInstance();
}
std::shared_ptr<EdgeFunction<IDETabulationProblemTestPlugin::l_t>>
IDETabulationProblemTestPlugin::getReturnEdgeFunction(n_t callSite,
                                                      f_t calleeFunction,
                                                      n_t exitStmt,
                                                      d_t exitNode, n_t reSite,
                                                      d_t retNode) {
  return EdgeIdentity<l_t>::getInstance();
}
std::shared_ptr<EdgeFunction<IDETabulationProblemTestPlugin::l_t>>
IDETabulationProblemTestPlugin::getCallToRetEdgeFunction(
    n_t callSite, d_t callNode, n_t retSite, d_t retSiteNode,
    std::set<f_t> callees) {
  return EdgeIdentity<l_t>::getInstance();
}
std::shared_ptr<EdgeFunction<IDETabulationProblemTestPlugin::l_t>>
IDETabulationProblemTestPlugin::getSummaryEdgeFunction(n_t curr, d_t currNode,
                                                       n_t succ, d_t succNode) {
  return nullptr;
}

IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::topElement() {
  return nullptr;
}
IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::bottomElement() {
  return getZeroValue();
}
IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::join(l_t lhs, l_t rhs) {
  if (lhs == rhs || rhs == topElement())
    return lhs;
  if (lhs == topElement())
    return rhs;
  return bottomElement();
}

} // namespace psr
