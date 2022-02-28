#ifndef MYIFDSPROBLEM_H_
#define MYIFDSPROBLEM_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h"

namespace psr {
class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;
} // namespace psr

struct MyFlowFact : public psr::FlowFactWrapper<const llvm::Value *> {

  using FlowFactWrapper::FlowFactWrapper;

  void print(std::ostream &OS,
             const llvm::Value *const &NonZeroFact) const override {
    OS << psr::llvmIRToShortString(NonZeroFact) << '\n';
  }
};

class MyIFDSProblem : public psr::IFDSTabulationProblemPlugin {
private:
  psr::FlowFactManager<MyFlowFact> FFManager;

public:
  MyIFDSProblem(const psr::ProjectIRDB *IRDB, const psr::LLVMTypeHierarchy *TH,
                const psr::LLVMBasedICFG *ICF, psr::LLVMPointsToInfo *PT,
                std::set<std::string> EntryPoints);

  ~MyIFDSProblem() override = default;

  const psr::FlowFact *createZeroValue() const override;

  FlowFunctionPtrType
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  FlowFunctionPtrType
  getCallFlowFunction(const llvm::Instruction *callSite,
                      const llvm::Function *destMthd) override;

  FlowFunctionPtrType
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitSite,
                     const llvm::Instruction *retSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  FlowFunctionPtrType
  getSummaryFlowFunction(const llvm::Instruction *callSite,
                         const llvm::Function *destMthd) override;

  std::map<const llvm::Instruction *, std::set<const psr::FlowFact *>>
  initialSeeds() override;
};

extern "C" std::unique_ptr<psr::IFDSTabulationProblemPlugin>
makeMyIFDSProblem(const psr::ProjectIRDB *IRDB,
                  const psr::LLVMTypeHierarchy *TH,
                  const psr::LLVMBasedICFG *ICF, psr::LLVMPointsToInfo *PT,
                  std::set<std::string> EntryPoints);

#endif
