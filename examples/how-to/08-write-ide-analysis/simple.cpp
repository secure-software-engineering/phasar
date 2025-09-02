#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace {

/// The domain for our analysis. We specialize the edge-value type l_t, i.e.,
/// the type of constant values that are assigned to constant integer
/// variables.
struct ExampleIDELinearConstantAnalysisDomain : psr::LLVMAnalysisDomainDefault {
  // We want to propagate constant integers. To make this domain a lattice, we
  // wrap it into psr::LatticeDomain, adding special values for TOP and BOTTOM.
  using l_t = psr::LatticeDomain<int64_t>;
};

/// To create a custom IDE analysis, we must create a subclass of the
/// IDETabulationProblem.
/// The utility class DefaultNoAliasIDEProblem implements
/// IDETabulationProblem and already provides some default flow-functions, so
/// that we can focus on the specifica of our analysis.
///
/// \note For simplicity, we don't handle aliasing in this example; however, you
/// can use DefaultAliasAwareIDEProblem to handle aliasing in most cases.
class ExampleLinearConstantAnalysis
    : public psr::DefaultNoAliasIDEProblem<
          ExampleIDELinearConstantAnalysisDomain> {
public:
  /// Constructor of the constant-analysis problem. Just forward all parameters
  /// to the base-class.
  ///
  /// The last parameter of the base-ctor denotes the special zero-value, aka.
  /// Λ, of the IDE problem. We use LLVMZeroValue for this.
  explicit ExampleLinearConstantAnalysis(const psr::LLVMProjectIRDB *IRDB,
                                         std::vector<std::string> EntryPoints)
      : DefaultNoAliasIDEProblem(IRDB, std::move(EntryPoints),
                                 psr::LLVMZeroValue::getInstance()) {}

  /// Provides the initial seeds, i.e., the <stmt, fact> pairs that are assumed
  /// to hold un-conditionally at the beginning of the analysis.
  /// Similar to IFDS, this is the start state that the IDE solver will use to
  /// start with.
  [[nodiscard]] psr::InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    psr::InitialSeeds<n_t, d_t, l_t> Seeds;

    psr::LLVMBasedCFG CFG;
    // Here, we just say that for all entry-functions in the EntryPoints, the
    // zero-value should hold at the very first statement.
    addSeedsForStartingPoints(EntryPoints, IRDB, CFG, Seeds, getZeroValue(),
                              bottomElement());

    return Seeds;
  };

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
      // Freshly allocated variables hold no constant value

      auto *AT = Alloca->getAllocatedType();
      if (AT->isIntegerTy() || psr::isIntegerLikeType(AT)) {
        return generateFromZero(Alloca);
      }
    }

    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      // Storing a constant integer.
      if (llvm::isa<llvm::ConstantInt>(Store->getValueOperand())) {
        return psr::strongUpdateStore(Store,
                                      psr::LLVMZeroValue::isLLVMZeroValue);
      }
    }

    // Leave everything else defaulted
    return this->DefaultNoAliasIDEProblem::getNormalFlowFunction(Curr, Succ);
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override {
    // We definitely want to re-use as much as possible from the default
    // call-flow-function
    auto DefaultFn =
        this->DefaultNoAliasIDEProblem::getCallFlowFunction(CallSite, DestFun);

    // If a constant int is passed as parameter, we need to generate the
    // parameter inside the callee from zero

    const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
    container_type Gen;
    for (const auto &[Arg, Param] : llvm::zip(Call->args(), DestFun->args())) {
      if (llvm::isa<llvm::ConstantInt>(Arg)) {
        Gen.insert(&Param);
      }
    }

    if (Gen.empty()) {
      // Nothing special, we can directly use the default call-FF
      return DefaultFn;
    }

    // Here, we combine both flow-functions:

    auto GenFn =
        generateManyFlowsAndKillAllOthers(std::move(Gen), getZeroValue());
    return unionFlows(std::move(DefaultFn), std::move(GenFn));
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) override {

    auto DefaultFn = this->DefaultNoAliasIDEProblem::getRetFlowFunction(
        CallSite, CalleeFun, ExitInst, RetSite);

    const auto *RetInst = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
    if (RetInst &&
        llvm::isa_and_present<llvm::ConstantInt>(RetInst->getReturnValue())) {
      // If we return a literal constant int, we must generate the corresponding
      // value at the call-site from zero, i.e., the CallSite itself in case of
      // LLVM's SSA form

      auto RetFn = generateFlowAndKillAllOthers(CallSite, getZeroValue());
      return unionFlows(std::move(DefaultFn), std::move(RetFn));
    }

    return DefaultFn;
  }

  // Fallback edge-function that models two composed edge-functions. We try to
  // use this as little as possible for performance reasons.
  struct LCAEdgeFunctionComposer : psr::EdgeFunctionComposer<l_t> {
    static psr::EdgeFunction<l_t>
    join(psr::EdgeFunctionRef<LCAEdgeFunctionComposer> This,
         const psr::EdgeFunction<l_t> &OtherFunction) {
      // Just use the default join.

      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }
      return psr::AllBottom<l_t>{};
    }
  };

  // The custom edge-function for binary operations
  struct BinOp {
    using l_t = ExampleIDELinearConstantAnalysisDomain::l_t;

    unsigned OpCode{};
    const llvm::ConstantInt *LeftConst{};
    const llvm::ConstantInt *RightConst{};

    // Utility function to make implementing computeTarget() easier
    [[nodiscard]] l_t executeBinOperation(l_t LVal, l_t RVal) const {
      auto *LopPtr = LVal.getValueOrNull();
      auto *RopPtr = RVal.getValueOrNull();

      if (!LopPtr || !RopPtr) {
        return psr::Bottom{};
      }

      auto Lop = *LopPtr;
      auto Rop = *RopPtr;

      // default initialize with BOTTOM (all information)
      int64_t Res;
      switch (OpCode) {
      case llvm::Instruction::Add:
        if (llvm::AddOverflow(Lop, Rop, Res)) {
          return psr::Bottom{};
        }
        return Res;

      case llvm::Instruction::Sub:
        if (llvm::SubOverflow(Lop, Rop, Res)) {
          return psr::Bottom{};
        }
        return Res;

      case llvm::Instruction::Mul:
        if (llvm::MulOverflow(Lop, Rop, Res)) {
          return psr::Bottom{};
        }
        return Res;

      case llvm::Instruction::UDiv:
      case llvm::Instruction::SDiv:
        if (Lop == std::numeric_limits<int64_t>::min() &&
            Rop == -1) { // Would produce and overflow, as the complement of min
                         // is not representable in a signed type.
          return psr::Bottom{};
        }
        if (Rop == 0) { // Division by zero is UB, so we return Bot
          return psr::Bottom{};
        }
        return Lop / Rop;

      case llvm::Instruction::URem:
      case llvm::Instruction::SRem:
        if (Rop == 0) { // Division by zero is UB, so we return Bot
          return psr::Bottom{};
        }
        return Lop % Rop;

      case llvm::Instruction::And:
        return Lop & Rop;
      case llvm::Instruction::Or:
        return Lop | Rop;
      case llvm::Instruction::Xor:
        return Lop ^ Rop;
      default:
        PHASAR_LOG_LEVEL(DEBUG, "Operation not supported by "
                                "IDELinearConstantAnalysis::"
                                "executeBinOperation()");
        return psr::Bottom{};
      }
    }

    // Utility function to aid the printing operator<<
    static char opToChar(const unsigned Op) {
      switch (Op) {
      case llvm::Instruction::Add:
        return '+';
      case llvm::Instruction::Sub:
        return '-';
      case llvm::Instruction::Mul:
        return '*';
      case llvm::Instruction::UDiv:
      case llvm::Instruction::SDiv:
        return '/';
      case llvm::Instruction::URem:
      case llvm::Instruction::SRem:
        return '%';
      case llvm::Instruction::And:
        return '&';
      case llvm::Instruction::Or:
        return '|';
      case llvm::Instruction::Xor:
        return '^';
      default:
        return ' ';
      }
    }

    // Required function that invokes the edge-function with an incoming value
    // that is substituted with the non-constant operand of the modeled binary
    // operation.
    [[nodiscard]] l_t computeTarget(l_t Source) const {
      if (LeftConst && RightConst) { // Simple constant-folding
        return executeBinOperation(LeftConst->getSExtValue(),
                                   RightConst->getSExtValue());
      }
      if (Source == psr::Bottom{}) {
        // Bottom is the top-value of our lattice. Whatever we do to it, it will
        // always stay Bottom
        return Source;
      }

      // Now, perform the linear arithmetic.
      // First, we have to check, which of the both operands is the literal
      if (RightConst) {
        // The right operand is the literal, so we plug in the incoming value as
        // left operand
        return executeBinOperation(Source, RightConst->getSExtValue());
      }
      if (LeftConst) {
        // The left operand is the literal, so we plug in the incoming value as
        // right operand
        return executeBinOperation(LeftConst->getSExtValue(), Source);
      }

      llvm::report_fatal_error(
          "Only linear constant propagation can be specified!");
    }

    // Optional function to expose the constant-ness of this edge-function for
    // optimization purposes
    [[nodiscard]] constexpr bool isConstant() const noexcept {
      // If both operands of this binary operation are literal constant, this
      // edge function always computes the same value
      return LeftConst && RightConst;
    }

    // Compose This edge-function with a different SecondFunction
    static psr::EdgeFunction<l_t>
    compose(psr::EdgeFunctionRef<BinOp> This,
            const psr::EdgeFunction<l_t> &SecondFunction) {
      // Trivial compositions can be defaulted:
      if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
        return Default;
      }

      // Here, we could for example add transformations like:
      // compose(BinOp{Add, ..., Const1}, BinOp{Add, ..., Const2}) -->
      // BinOp{Add, ..., Const1 + Const2}

      // Fallback for when we don't know better:
      return LCAEdgeFunctionComposer{This, SecondFunction};
    }

    // Join This edge-function dith a different OtherFunction, going in the
    // EF-lattice towards AllBottom.
    static psr::EdgeFunction<l_t>
    join(psr::EdgeFunctionRef<BinOp> This,
         const psr::EdgeFunction<l_t> &OtherFunction) {
      // Trivial joins can be defaulted:
      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }

      // Here we could, e.g., check whether the two edge functions are
      // semantically equivalent, althouth different and then return one of them

      // Sound fallback in case, we don't know better
      return psr::AllBottom<l_t>{};
    }

    constexpr bool operator==(const BinOp &Other) const noexcept {
      return OpCode == Other.OpCode && LeftConst == Other.LeftConst &&
             RightConst == Other.RightConst;
    }

    // Printing. optional
    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const BinOp &Bop) {
      OS << "BinOp[";
      if (Bop.LeftConst) {
        OS << *Bop.LeftConst;
      } else {
        OS << 'x';
      }

      OS << ' ' << opToChar(Bop.OpCode) << ' ';
      if (Bop.LeftConst) {
        OS << *Bop.LeftConst;
      } else {
        OS << 'x';
      }

      return OS;
    }
  };

  psr::EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                               n_t /*Succ*/,
                                               d_t SuccNode) override {
    if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
      // Handle the two cases, where we generate facts from zero:

      if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
        // Freshly allocated variables hold no constant value
        return psr::AllBottom<l_t>{};
      }

      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {

        // Storing a constant integer.
        const auto *ConstOperand =
            llvm::cast<llvm::ConstantInt>(Store->getValueOperand());
        return psr::ConstantEdgeFunction<l_t>{ConstOperand->getSExtValue()};
      }
    }

    // Handle binary operations. The corresponding flow-function is defaulted.
    if (llvm::isa<llvm::BinaryOperator>(Curr) && SuccNode == Curr &&
        CurrNode != SuccNode) {
      unsigned Op = Curr->getOpcode();
      auto *Lop = Curr->getOperand(0);
      auto *Rop = Curr->getOperand(1);
      // For non linear constant computation we propagate bottom
      if ((CurrNode == Lop && !llvm::isa<llvm::ConstantInt>(Rop)) ||
          (CurrNode == Rop && !llvm::isa<llvm::ConstantInt>(Lop))) {
        return psr::AllBottom<l_t>{};
      }

      // Attach the arithmetic transformer to this edge
      return BinOp{Op, llvm::dyn_cast<llvm::ConstantInt>(Lop),
                   llvm::dyn_cast<llvm::ConstantInt>(Rop)};
    }

    // Pass everything else as identity
    return psr::EdgeIdentity<l_t>{};
  }

  psr::EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                             f_t /*DestinationFunction*/,
                                             d_t DestNode) override {
    if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
      // If a constant int is passed as parameter, we need to generate the
      // parameter inside the callee from zero
      const auto *DestParam = llvm::cast<llvm::Argument>(DestNode);
      const auto *ConstOperand = llvm::cast<llvm::ConstantInt>(
          CallSite->getOperand(DestParam->getArgNo()));
      return psr::ConstantEdgeFunction<l_t>{ConstOperand->getSExtValue()};
    }

    // Pass everything else as identity
    return psr::EdgeIdentity<l_t>{};
  }

  psr::EdgeFunction<l_t>
  getReturnEdgeFunction(n_t CallSite, f_t /*CalleeFunction*/, n_t ExitStmt,
                        d_t ExitNode, n_t /*RetSite*/, d_t RetNode) override {
    if (isZeroValue(ExitNode) && RetNode == CallSite) {
      // If we return a literal constant int, we must generate the corresponding
      // value at the call-site from zero, i.e., the CallSite itself in case of
      // LLVM's SSA form
      const auto *RetVal =
          llvm::cast<llvm::ReturnInst>(ExitStmt)->getReturnValue();
      return psr::ConstantEdgeFunction<l_t>{
          llvm::cast<llvm::ConstantInt>(RetVal)->getSExtValue()};
    }

    // Pass everything else as identity
    return psr::EdgeIdentity<l_t>{};
  }

  psr::EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) override {
    // The call-to-return edge-function handles facts that are not affected by
    // the call. This is usually the identity function.
    return psr::EdgeIdentity<l_t>{};
  }
};
} // namespace

// Invoke the analysis the same way as explained in 05-run-ide-analysis:
int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: write-ide-analysis-simple <LLVM-IR file>\n";
    return 1;
  }

  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"});

  ExampleLinearConstantAnalysis LCAProblem(&IRDB, {"main"});

  auto Results = psr::solveIDEProblem(LCAProblem, ICFG);

  // After we have solved the LCAProblem, we can now inspect the detected
  // constants:

  const auto *MainF = IRDB.getFunctionDefinition("main");
  if (!MainF) {
    llvm::errs() << "Required function 'main' not found\n";
    return 1;
  }

  const auto *ExitOfMain = psr::getAllExitPoints(MainF).front();

  // Get the analysis results right **after** main's return statement
  const auto &AllConstantsAtMainExit = Results.resultsAt(ExitOfMain);

  llvm::outs() << "Detected constants at " << psr::llvmIRToString(ExitOfMain)
               << ":\n";
  for (const auto &[LLVMVar, ConstVal] : AllConstantsAtMainExit) {
    llvm::outs() << "  " << psr::llvmIRToString(LLVMVar) << "\n  --> ";
    if (ConstVal.isBottom()) {
      // A "bottom" value here means that the analysis does not know the value
      // at this point and that any value may be possible.

      llvm::outs() << "<not constant>\n\n";
    } else {
      llvm::outs() << ConstVal << "\n\n";
    }
  }
}
