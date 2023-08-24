#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Printer.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

std::string psr::NToString(const llvm::Value *V) { return llvmIRToString(V); }
std::string psr::NToString(const llvm::Instruction *V) {
  return llvmIRToString(V);
}

std::string psr::DToString(const llvm::Value *V) { return llvmIRToString(V); }

llvm::StringRef psr::FToString(const llvm::Function *V) { return V->getName(); }
