#include "IntraMonoFullConstantPropagation.h"

IntraMonoFullConstantPropagation::IntraMonoFullConstantPropagation(
    LLVMBasedCFG &Cfg, const llvm::Function *F)
    : IntraMonotoneProblem<const llvm::Instruction *, DFF,
                           const llvm::Function *, LLVMBasedCFG &>(Cfg, F) {}

MonoSet<IntraMonoFullConstantPropagation::DFF>
IntraMonoFullConstantPropagation::join(const MonoSet<DFF> &Lhs,
                                       const MonoSet<DFF> &Rhs) {
  MonoSet<IntraMonoFullConstantPropagation::DFF> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoFullConstantPropagation::sqSubSetEqual(const MonoSet<DFF> &Lhs,
                                                     const MonoSet<DFF> &Rhs) {
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<IntraMonoFullConstantPropagation::DFF>
IntraMonoFullConstantPropagation::flow(const llvm::Instruction *S,
                                       const MonoSet<DFF> &In) {
  return MonoSet<IntraMonoFullConstantPropagation::DFF>();
}

MonoMap<const llvm::Instruction *,
        MonoSet<IntraMonoFullConstantPropagation::DFF>>
IntraMonoFullConstantPropagation::initialSeeds() {
  return MonoMap<const llvm::Instruction *,
                 MonoSet<IntraMonoFullConstantPropagation::DFF>>();
}

string IntraMonoFullConstantPropagation::DtoString(
    pair<const llvm::Value *, unsigned> d) {
  string s = "< " + llvmIRToString(d.first);
  s += ", " + to_string(d.second) + " >";
  return s;
}
