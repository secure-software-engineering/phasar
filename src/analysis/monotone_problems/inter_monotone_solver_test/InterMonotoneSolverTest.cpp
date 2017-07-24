#include "InterMonotoneSolverTest.hh"

InterMonotoneSolverTest::InterMonotoneSolverTest(LLVMBasedICFG &Icfg)
    : InterMonotoneProblem<const llvm::Instruction *, const llvm::Value *,
                           const llvm::Function *, LLVMBasedICFG &>(Icfg),
      ICFG(Icfg) {}

MonoSet<const llvm::Value *>
InterMonotoneSolverTest::join(const MonoSet<const llvm::Value *> &Lhs,
                              const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonotoneSolverTest::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool InterMonotoneSolverTest::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonotoneSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
InterMonotoneSolverTest::normalFlow(const llvm::Instruction *Stmt,
                                    const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonotoneSolverTest::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  return Result;
}

MonoSet<const llvm::Value *>
InterMonotoneSolverTest::callFlow(const llvm::Instruction *CallSite,
                                  const llvm::Function *Callee,
                                  const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonotoneSolverTest::callFlow()\n";
  return In;
}

MonoSet<const llvm::Value *> InterMonotoneSolverTest::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *RetStmt, const llvm::Instruction *RetSite,
    const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonotoneSolverTest::returnFlow()\n";
  return In;
}

MonoSet<const llvm::Value *>
InterMonotoneSolverTest::callToRetFlow(const llvm::Instruction *CallSite,
                                       const llvm::Instruction *RetSite,
                                       const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonotoneSolverTest::callToRetFlow()\n";
  return In;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
InterMonotoneSolverTest::initialSeeds() {
  cout << "InterMonotoneSolverTest::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>> Seeds;
  Seeds.insert(
      std::make_pair(&main->front().front(), MonoSet<const llvm::Value *>()));
  return Seeds;
}

string InterMonotoneSolverTest::D_to_string(const llvm::Value *d) {
  return llvmIRToString(d);
}
