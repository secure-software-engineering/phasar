#ifndef MYIFDSPROBLEM_H_
#define MYIFDSPROBLEM_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>
#include <map>
#include <memory>
#include <set>
#include <vector>

class MyIFDSProblem : public psr::IFDSTabulationProblemPlugin {
 public:
  MyIFDSProblem(psr::LLVMBasedICFG &I, std::vector<std::string> EntryPoints);
  ~MyIFDSProblem() = default;
  std::shared_ptr<psr::FlowFunction<const llvm::Value *>> getNormalFlowFunction(
      const llvm::Instruction *curr, const llvm::Instruction *succ) override;

  std::shared_ptr<psr::FlowFunction<const llvm::Value *>> getCallFlowFunction(
      const llvm::Instruction *callStmt,
      const llvm::Function *destMthd) override;

  std::shared_ptr<psr::FlowFunction<const llvm::Value *>> getRetFlowFunction(
      const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
      const llvm::Instruction *exitStmt,
      const llvm::Instruction *retSite) override;

  std::shared_ptr<psr::FlowFunction<const llvm::Value *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           set<const llvm::Function *> callees) override;

  std::shared_ptr<psr::FlowFunction<const llvm::Value *>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destMthd) override;

  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
  initialSeeds() override;
};

extern "C" std::unique_ptr<psr::IFDSTabulationProblemPlugin> makeMyIFDSProblem(
    psr::LLVMBasedICFG &I, std::vector<std::string> EntryPoints);

#endif
