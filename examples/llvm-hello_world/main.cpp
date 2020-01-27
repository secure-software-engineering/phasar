#include <iostream>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "usage: <prog> <IR file>\n";
    return 1;
  }
  // parse an IR file into an LLVM module
  llvm::SMDiagnostic Diag;
  std::unique_ptr<llvm::LLVMContext> C(new llvm::LLVMContext);
  std::unique_ptr<llvm::Module> M = llvm::parseIRFile(argv[1], Diag, *C);
  // check if the module is alright
  bool broken_debug_info = false;
  if (M.get() == nullptr ||
      llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
    llvm::errs() << "error: module not valid\n";
    return 1;
  }
  if (broken_debug_info) {
    llvm::errs() << "caution: debug info is broken\n";
  }
  auto F = M->getFunction("main");
  if (!F) {
    llvm::errs() << "error: could not find function 'main'\n";
    return 1;
  }
  llvm::outs() << "iterate instructions of function: '" << F->getName()
               << "'\n";
  for (auto &BB : *F) {
    for (auto &I : BB) {
      I.print(llvm::outs());
      llvm::outs() << '\n';
      // TODO: Analyze instruction 'I' here.
      // (see http://llvm.org/doxygen/classllvm_1_1Instruction.html)
      // For instance, let's check if 'I' is an 'AllocaInst':
      //
      // if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      //    At this point you can use the memberfunctions of llvm::AllocaInst
      //    on the pointer variable Alloc as it is a more specialized sub-type
      //    of llvm::Instruction.
      //    llvm::outs() << "Found an alloca instruction!\n";
      // }
      //
      // For the further tasks you may like to familiarize yourself with the
      // sub-instructions 'llvm::LoadInst' and 'llvm::StoreInst' as well as
      // 'llvm::CallInst'. Use if-constructs like shown in the above.
      //
      // If you just want to figure out if a variable is an instance of a
      // certain type you can use 'llvm::isa<>()' like this:
      //
      // if (llvm::isa<llvm::CallInst>(&I)) {
      //   llvm::outs() << "Found a CallInst!\n";
      // }
      //
      // Both, 'llvm::isa' and 'llvm::dyn_cast' only work for variables within
      // the universe of LLVM. These features have been crafted for efficiency
      // and only require a single look-up; so do not be afraid to use them.
      // If you need to find inheritance relationsships in non-LLVM context use
      // C++'s classical 'dynamic_cast'.
    }
  }
  llvm::llvm_shutdown();
  return 0;
}
