#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalICFG.h>

namespace psr {
LLVMBasedVariationalICFG::LLVMBasedVariationalICFG(LLVMTypeHierarchy &STH,
                                                   ProjectIRDB &IRDB)
    : icfg(STH, IRDB) {}

LLVMBasedVariationalICFG::LLVMBasedVariationalICFG(
    LLVMTypeHierarchy &STH, ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    const std::set<std::string> &EntryPoints)
    : icfg(STH, IRDB, CGType, EntryPoints) {}

LLVMBasedVariationalICFG::LLVMBasedVariationalICFG(
    LLVMTypeHierarchy &STH, ProjectIRDB &IRDB, const llvm::Module &M,
    CallGraphAnalysisType CGType, std::set<std::string> EntryPoints)
    : icfg(STH, IRDB, M, CGType, EntryPoints) {}

#pragma region LLVMBasedICFG
std::set<const llvm::Function *>
LLVMBasedVariationalICFG::getAllFunctions() const {
  return icfg.getAllFunctions();
}

bool LLVMBasedVariationalICFG::isIndirectFunctionCall(
    const llvm::Instruction *n) const {
  return icfg.isIndirectFunctionCall(n);
}

bool LLVMBasedVariationalICFG::isVirtualFunctionCall(
    const llvm::Instruction *n) const {
  return icfg.isVirtualFunctionCall(n);
}

const llvm::Function *
LLVMBasedVariationalICFG::getFunction(const std::string &fun) const {
  return icfg.getFunction(fun);
}

std::set<const llvm::Function *>
LLVMBasedVariationalICFG::getCalleesOfCallAt(const llvm::Instruction *n) const {
  return icfg.getCalleesOfCallAt(n);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::getCallersOf(const llvm::Function *m) const {
  return icfg.getCallersOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::getCallsFromWithin(const llvm::Function *m) const {
  return icfg.getCallsFromWithin(m);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::getStartPointsOf(const llvm::Function *m) const {
  return icfg.getStartPointsOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::getExitPointsOf(const llvm::Function *fun) const {
  return icfg.getExitPointsOf(fun);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::getReturnSitesOfCallAt(
    const llvm::Instruction *n) const {
  return icfg.getReturnSitesOfCallAt(n);
}

bool LLVMBasedVariationalICFG::isCallStmt(const llvm::Instruction *stmt) const {
  return icfg.isCallStmt(stmt);
}

std::set<const llvm::Instruction *>
LLVMBasedVariationalICFG::allNonCallStartNodes() const {
  return icfg.allNonCallStartNodes();
}

const llvm::Instruction *
LLVMBasedVariationalICFG::getLastInstructionOf(const std::string &name) {
  return icfg.getLastInstructionOf(name);
}

std::vector<const llvm::Instruction *>
LLVMBasedVariationalICFG::getAllInstructionsOfFunction(
    const std::string &name) {
  return icfg.getAllInstructionsOfFunction(name);
}

void LLVMBasedVariationalICFG::mergeWith(const LLVMBasedICFG &other) {
  icfg.mergeWith(other);
}

bool LLVMBasedVariationalICFG::isPrimitiveFunction(const std::string &name) {
  return icfg.isPrimitiveFunction(name);
}

void LLVMBasedVariationalICFG::print() { icfg.print(); }

void LLVMBasedVariationalICFG::printAsDot(const std::string &filename) {
  icfg.printAsDot(filename);
}

void LLVMBasedVariationalICFG::printInternalPTGAsDot(
    const std::string &filename) {
  icfg.printInternalPTGAsDot(filename);
}

nlohmann::json LLVMBasedVariationalICFG::getAsJson() const {
  return icfg.getAsJson();
}

unsigned LLVMBasedVariationalICFG::getNumOfVertices() {
  return icfg.getNumOfVertices();
}

unsigned LLVMBasedVariationalICFG::getNumOfEdges() {
  return icfg.getNumOfEdges();
}

void LLVMBasedVariationalICFG::exportPATBCJSON() { icfg.exportPATBCJSON(); }

const PointsToGraph &LLVMBasedVariationalICFG::getWholeModulePTG() const {
  return icfg.getWholeModulePTG();
}

std::vector<std::string>
LLVMBasedVariationalICFG::getDependencyOrderedFunctions() {
  return icfg.getDependencyOrderedFunctions();
}
#pragma endregion
} // namespace psr