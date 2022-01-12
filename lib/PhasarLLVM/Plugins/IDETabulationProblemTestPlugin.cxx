#include <iostream>
#include <utility>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

#include "IDETabulationProblemTestPlugin.h"

using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IDETabulationProblemPlugin> makeIDETabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints) {
  return unique_ptr<IDETabulationProblemPlugin>(
      new IDETabulationProblemTestPlugin(IRDB, TH, ICF, PT,
                                         std::move(EntryPoints)));
}

__attribute__((constructor)) void init() {
  cout << "init - IDETabulationProblemTestPlugin\n";
  IDETabulationProblemPluginFactory["ide_testplugin"] =
      &makeIDETabulationProblemTestPlugin;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IDETabulationProblemTestPlugin\n";
}

IDETabulationProblemTestPlugin::IDETabulationProblemTestPlugin(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblemPlugin(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  ZeroValue = IDETabulationProblemTestPlugin::createZeroValue();
}

const FlowFact *IDETabulationProblemTestPlugin::createZeroValue() const {
  static auto *Zero = new ValueFlowFactWrapper(nullptr);
  return Zero;
}

IDETabulationProblemTestPlugin::FlowFunctionPtrType
IDETabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction * /*Curr*/, const llvm::Instruction * /*Succ*/) {
  return Identity<const FlowFact *>::getInstance();
}

IDETabulationProblemTestPlugin::FlowFunctionPtrType
IDETabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return Identity<const FlowFact *>::getInstance();
}

IDETabulationProblemTestPlugin::FlowFunctionPtrType
IDETabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*CalleeFun*/,
    const llvm::Instruction * /*ExitSite*/,
    const llvm::Instruction * /*RetSite*/) {
  return Identity<const FlowFact *>::getInstance();
}

IDETabulationProblemTestPlugin::FlowFunctionPtrType
IDETabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Instruction * /*RetSite*/,
    set<const llvm::Function *> /*Callees*/) {
  return Identity<const FlowFact *>::getInstance();
}

IDETabulationProblemTestPlugin::FlowFunctionPtrType
IDETabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction * /*CallSite*/,
    const llvm::Function * /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDETabulationProblemTestPlugin::n_t,
             IDETabulationProblemTestPlugin::d_t,
             IDETabulationProblemTestPlugin::l_t>
IDETabulationProblemTestPlugin::initialSeeds() {
  cout << "IDETabulationProblemTestPlugin::initialSeeds()\n";
  InitialSeeds<IDETabulationProblemTestPlugin::n_t,
               IDETabulationProblemTestPlugin::d_t,
               IDETabulationProblemTestPlugin::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), bottomElement());
  }
  return Seeds;
}

IDETabulationProblemTestPlugin::EdgeFunctionPtrType
IDETabulationProblemTestPlugin::getNormalEdgeFunction(n_t /*Curr*/,
                                                      d_t /*CurrNode*/,
                                                      n_t /*Succ*/,
                                                      d_t /*SuccNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}
IDETabulationProblemTestPlugin::EdgeFunctionPtrType
IDETabulationProblemTestPlugin::getCallEdgeFunction(n_t /*CallSite*/,
                                                    d_t /*SrcNode*/,
                                                    f_t /*DestinationFunction*/,
                                                    d_t /*DestNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}
IDETabulationProblemTestPlugin::EdgeFunctionPtrType
IDETabulationProblemTestPlugin::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeFunction*/, n_t /*ExitSite*/,
    d_t /*ExitNode*/, n_t /*RetSite*/, d_t /*RetNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}
IDETabulationProblemTestPlugin::EdgeFunctionPtrType
IDETabulationProblemTestPlugin::getCallToRetEdgeFunction(
    n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/, d_t /*RetSiteNode*/,
    std::set<f_t> /*Callees*/) {
  return EdgeIdentity<l_t>::getInstance();
}
IDETabulationProblemTestPlugin::EdgeFunctionPtrType
IDETabulationProblemTestPlugin::getSummaryEdgeFunction(n_t /*Curr*/,
                                                       d_t /*CurrNode*/,
                                                       n_t /*Succ*/,
                                                       d_t /*SuccNode*/) {
  return nullptr;
}

IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::topElement() {
  return EFManager.getOrCreateEdgeFact(-1);
}
IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::bottomElement() {
  return EFManager.getOrCreateEdgeFact(0);
}
IDETabulationProblemTestPlugin::l_t
IDETabulationProblemTestPlugin::join(l_t Lhs, l_t Rhs) {
  if (Lhs == Rhs || Rhs == topElement()) {
    return Lhs;
  }
  if (Lhs == topElement()) {
    return Rhs;
  }
  return bottomElement();
}

} // namespace psr
