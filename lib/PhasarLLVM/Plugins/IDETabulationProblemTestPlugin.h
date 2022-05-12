#ifndef SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_H_
#define SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_H_

#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h"

namespace psr {

struct IntEdgeFactWrapper : public EdgeFactWrapper<int> {
  using EdgeFactWrapper::EdgeFactWrapper;

  inline void print(llvm::raw_ostream &OS) const override { OS << get(); }
};
struct ValueFlowFactWrapper : public FlowFactWrapper<const llvm::Value *> {
  using FlowFactWrapper::FlowFactWrapper;

  inline void print(llvm::raw_ostream &OS,
                    const llvm::Value *const &NonzeroFact) const override {
    OS << llvmIRToShortString(NonzeroFact) << '\n';
  }
};

class IDETabulationProblemTestPlugin : public IDETabulationProblemPlugin {
  EdgeFactManager<IntEdgeFactWrapper> EFManager;

public:
  IDETabulationProblemTestPlugin(const ProjectIRDB *IRDB,
                                 const LLVMTypeHierarchy *TH,
                                 const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                                 std::set<std::string> EntryPoints);

  ~IDETabulationProblemTestPlugin() override = default;

  [[nodiscard]] const FlowFact *createZeroValue() const override;

  FlowFunctionPtrType
  getNormalFlowFunction(const llvm::Instruction *Curr,
                        const llvm::Instruction *Succ) override;

  FlowFunctionPtrType
  getCallFlowFunction(const llvm::Instruction *CallSite,
                      const llvm::Function *DestFun) override;

  FlowFunctionPtrType
  getRetFlowFunction(const llvm::Instruction *CallSite,
                     const llvm::Function *CalleeFun,
                     const llvm::Instruction *ExitStmt,
                     const llvm::Instruction *RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(const llvm::Instruction *CallSite,
                           const llvm::Instruction *RetSite,
                           std::set<const llvm::Function *> Callees) override;

  FlowFunctionPtrType
  getSummaryFlowFunction(const llvm::Instruction *CallSite,
                         const llvm::Function *DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  EdgeFunctionPtrType getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                            d_t SuccNode) override;
  EdgeFunctionPtrType getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                          f_t DestinationFunction,
                                          d_t DestNode) override;
  EdgeFunctionPtrType getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                            n_t ExitStmt, d_t ExitNode,
                                            n_t RetSite, d_t RetNode) override;
  EdgeFunctionPtrType getCallToRetEdgeFunction(n_t CallSite, d_t CallNode,
                                               n_t RetSite, d_t RetSiteNode,
                                               std::set<f_t> Callees) override;
  EdgeFunctionPtrType getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                             d_t SuccNode) override;

  l_t topElement() override;
  l_t bottomElement() override;
  l_t join(l_t Lhs, l_t Rhs) override;
};

extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemtestPlugin(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints);

} // namespace psr
#endif
