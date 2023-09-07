/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

#include <limits>
#include <memory>
#include <utility>

namespace psr {

namespace lca {
// Custom EdgeFunction declarations

using l_t = IDELinearConstantAnalysisDomain::l_t;
using d_t = IDELinearConstantAnalysisDomain::d_t;

// For debug purpose only
static unsigned CurrGenConstantId = 0; // NOLINT
static unsigned CurrBinaryId = 0;      // NOLINT

struct LCAEdgeFunctionComposer : EdgeFunctionComposer<l_t> {

  static EdgeFunction<l_t> join(EdgeFunctionRef<LCAEdgeFunctionComposer> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
      return Default;
    }
    return AllBottom<l_t>{};
  }
};

using GenConstant = ConstantEdgeFunction<l_t>;

/**
 * The following binary operations are computed:
 *  - addition
 *  - subtraction
 *  - multiplication
 *  - division (signed/unsinged)
 *  - remainder (signed/unsinged)
 *
 * @brief Computes the result of a binary operation.
 * @param op operator
 * @param lop left operand
 * @param rop right operand
 * @return Result of binary operation
 */
static l_t executeBinOperation(unsigned Op, l_t LVal, l_t RVal) {
  auto *LopPtr = std::get_if<int64_t>(&LVal);
  auto *RopPtr = std::get_if<int64_t>(&RVal);

  if (!LopPtr || !RopPtr) {
    return Bottom{};
  }

  auto Lop = *LopPtr;
  auto Rop = *RopPtr;

  // default initialize with BOTTOM (all information)
  int64_t Res;
  switch (Op) {
  case llvm::Instruction::Add:
    if (llvm::AddOverflow(Lop, Rop, Res)) {
      return Bottom{};
    }
    return Res;

  case llvm::Instruction::Sub:
    if (llvm::SubOverflow(Lop, Rop, Res)) {
      return Bottom{};
    }
    return Res;

  case llvm::Instruction::Mul:
    if (llvm::MulOverflow(Lop, Rop, Res)) {
      return Bottom{};
    }
    return Res;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    if (Lop == std::numeric_limits<int64_t>::min() &&
        Rop == -1) { // Would produce and overflow, as the complement of min is
                     // not representable in a signed type.
      return Bottom{};
    }
    if (Rop == 0) { // Division by zero is UB, so we return Bot
      return Bottom{};
    }
    return Lop / Rop;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    if (Rop == 0) { // Division by zero is UB, so we return Bot
      return Bottom{};
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
    return Bottom{};
  }
}

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

struct BinOp {
  using l_t = lca::l_t;

  unsigned EdgeFunctionID, Op;
  d_t Lop, Rop, CurrNode;

  explicit BinOp(unsigned Op, d_t Lop, d_t Rop, d_t CurrNode) noexcept
      : EdgeFunctionID(++CurrBinaryId), Op(Op), Lop(Lop), Rop(Rop),
        CurrNode(CurrNode) {}

  l_t computeTarget(l_t Source) const {
    static_assert(IsEdgeFunction<BinOp>);

    PHASAR_LOG_LEVEL(DEBUG, "Left Op   : " << llvmIRToString(Lop));
    PHASAR_LOG_LEVEL(DEBUG, "Right Op  : " << llvmIRToString(Rop));
    PHASAR_LOG_LEVEL(DEBUG, "Curr Node : " << llvmIRToString(CurrNode));
    PHASAR_LOG_LEVEL(DEBUG, ' ');

    if (LLVMZeroValue::isLLVMZeroValue(CurrNode) &&
        llvm::isa<llvm::ConstantInt>(Lop) &&
        llvm::isa<llvm::ConstantInt>(Rop)) {
      const auto *Lic = llvm::cast<llvm::ConstantInt>(Lop);
      const auto *Ric = llvm::cast<llvm::ConstantInt>(Rop);
      return executeBinOperation(Op, Lic->getSExtValue(), Ric->getSExtValue());
    }
    if (Source == Bottom{}) {
      return Source;
    }
    if (Lop == CurrNode && llvm::isa<llvm::ConstantInt>(Rop)) {
      const auto *Ric = llvm::cast<llvm::ConstantInt>(Rop);
      return executeBinOperation(Op, Source, Ric->getSExtValue());
    }
    if (Rop == CurrNode && llvm::isa<llvm::ConstantInt>(Lop)) {
      const auto *Lic = llvm::cast<llvm::ConstantInt>(Lop);
      return executeBinOperation(Op, Lic->getSExtValue(), Source);
    }

    llvm::report_fatal_error(
        "Only linear constant propagation can be specified!");
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<BinOp> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }

    // TODO: Optimize Binop::composeWith(BinOp)

    return LCAEdgeFunctionComposer{This, SecondFunction};
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<BinOp> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
      return Default;
    }
    return AllBottom<l_t>{};
  }

  bool operator==(const BinOp &BOP) const noexcept {
    return BOP.Op == Op && BOP.Lop == Lop && BOP.Rop == Rop;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const BinOp &BOP) {
    if (const auto *LIC = llvm::dyn_cast<llvm::ConstantInt>(BOP.Lop)) {
      OS << LIC->getSExtValue();
    } else {
      OS << "ID:" << getMetaDataID(BOP.Lop);
    }
    OS << ' ' << opToChar(BOP.Op) << ' ';
    if (const auto *RIC = llvm::dyn_cast<llvm::ConstantInt>(BOP.Rop)) {
      OS << RIC->getSExtValue();
    } else {
      OS << "ID:" << getMetaDataID(BOP.Rop);
    }

    return OS;
  }
};
} // namespace lca

IDELinearConstantAnalysis::IDELinearConstantAnalysis(
    const LLVMProjectIRDB *IRDB, const LLVMBasedICFG *ICF,
    std::vector<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      ICF(ICF) {
  assert(ICF != nullptr);
}

IDELinearConstantAnalysis::~IDELinearConstantAnalysis() {
  lca::CurrGenConstantId = 0;
  lca::CurrBinaryId = 0;
}

// Start formulating our analysis by specifying the parts required for IFDS

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getNormalFlowFunction(n_t Curr, n_t /*Succ*/) {
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    auto *AT = Alloca->getAllocatedType();
    if (AT->isIntegerTy() || isIntegerLikeType(AT)) {
      return generateFromZero(Alloca);
    }
  }

  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e., kills previous fact (= pointer operand).
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    d_t ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      return strongUpdateStore(Store, LLVMZeroValue::isLLVMZeroValue);
    }

    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      return strongUpdateStore(Store);
    }
  }

  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    if (GEP->getResultElementType()->isIntegerTy()) {
      const auto *Op = GEP->getPointerOperand();
      return generateFlow(GEP, Op);
    }
  }
  // check load instructions
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    // only consider i32 load
    if (Load->getType()->isIntegerTy()) {
      return generateFlowIf(Load, [Load](d_t Source) {
        return Source == Load->getPointerOperand();
      });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(Curr)) {
    auto *Lop = Curr->getOperand(0);
    auto *Rop = Curr->getOperand(1);
    return generateFlowIf(Curr, [this, Lop, Rop](d_t Source) {
      /// Intentionally include nonlinear operations here for being able to
      /// explicitly set them to BOTTOM in the edge function
      return (Lop == Source) || (Rop == Source) ||
             (isZeroValue(Source) && llvm::isa<llvm::ConstantInt>(Lop) &&
              llvm::isa<llvm::ConstantInt>(Rop));
    });
  }

  if (const auto *Extract = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {
    const auto *Agg = Extract->getAggregateOperand();

    /// We are extracting the result of a BinaryOpIntrinsic
    /// The first parameter holds the resulting integer if
    /// no error occured during the operation
    if (const auto *BinIntrinsic =
            llvm::dyn_cast<llvm::BinaryOpIntrinsic>(Agg)) {
      if (Extract->getType()->isIntegerTy()) {
        return generateFlow(Curr, Agg);
      }
    }
  }

  return identityFlow();
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getCallFlowFunction(n_t CallSite, f_t DestFun) {
  // Map the actual parameters into the formal parameters
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    if (!DestFun->isDeclaration()) {
      return mapFactsToCallee(CS, DestFun, [](d_t Arg, d_t Source) {
        return Arg == Source || (LLVMZeroValue::isLLVMZeroValue(Source) &&
                                 llvm::isa<llvm::ConstantInt>(Arg));
      });
    }
  }
  // Pass everything else as identity
  return identityFlow();
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                              n_t ExitInst, n_t /*RetSite*/) {
  return mapFactsToCaller(
      llvm::cast<llvm::CallBase>(CallSite), ExitInst,
      [](d_t Arg, d_t Source) {
        return Arg == Source && Arg->getType()->isPointerTy();
      },
      [](d_t RetVal, d_t Source) {
        return RetVal == Source || (LLVMZeroValue::isLLVMZeroValue(Source) &&
                                    llvm::isa<llvm::ConstantInt>(RetVal));
      });
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> Callees) {
  if (llvm::all_of(Callees, [](f_t Fun) { return Fun->isDeclaration(); })) {
    return identityFlow();
  }

  return mapFactsAlongsideCallSite(
      llvm::cast<llvm::CallBase>(CallSite),
      [](d_t Arg) { return !Arg->getType()->isPointerTy(); },
      /*PropagateGlobals*/ false);
}

InitialSeeds<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
             IDELinearConstantAnalysis::l_t>
IDELinearConstantAnalysis::initialSeeds() {
  InitialSeeds<n_t, d_t, l_t> Seeds;

  forallStartingPoints(EntryPoints, ICF, [this, &Seeds](n_t SP) {
    Seeds.addSeed(SP, getZeroValue(), bottomElement());
    // Generate global integer-typed variables using generalized initial seeds

    for (const auto &G : IRDB->getModule()->globals()) {
      if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
        if (GV->hasInitializer()) {
          if (const auto *ConstInt =
                  llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer())) {
            Seeds.addSeed(SP, GV, ConstInt->getSExtValue());
          }
        }
      }
    }
  });

  return Seeds;
}

IDELinearConstantAnalysis::FlowFunctionPtrType
IDELinearConstantAnalysis::getSummaryFlowFunction(n_t Curr, f_t /*CalleeFun*/) {

  if (const auto *BinIntrinsic =
          llvm::dyn_cast<llvm::BinaryOpIntrinsic>(Curr)) {
    auto *Lop = BinIntrinsic->getLHS();
    auto *Rop = BinIntrinsic->getRHS();

    return generateFlowIf(BinIntrinsic, [this, Lop, Rop](d_t Source) {
      /// Intentionally include nonlinear operations here for being able to
      /// explicitly set them to BOTTOM in the edge function
      return (Lop == Source) || (Rop == Source) ||
             (isZeroValue(Source) && llvm::isa<llvm::ConstantInt>(Lop) &&
              llvm::isa<llvm::ConstantInt>(Rop));
    });
  }
  return nullptr;
}

IDELinearConstantAnalysis::d_t
IDELinearConstantAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantAnalysis::isZeroValue(d_t Fact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

// In addition provide specifications for the IDE parts

EdgeFunction<lca::l_t>
IDELinearConstantAnalysis::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                 n_t /*Succ*/, d_t SuccNode) {
  if (isZeroValue(CurrNode) && isZeroValue(SuccNode)) {
    return EdgeIdentity<l_t>{};
  }

  // ALL_BOTTOM for zero value
  if ((llvm::isa<llvm::AllocaInst>(Curr) && isZeroValue(CurrNode))) {
    PHASAR_LOG_LEVEL(DEBUG, "Case: Zero value.");
    PHASAR_LOG_LEVEL(DEBUG, ' ');
    return AllBottom<l_t>{};
  }

  // Check store instruction
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    d_t PointerOperand = Store->getPointerOperand();
    d_t ValueOperand = Store->getValueOperand();
    if (PointerOperand == SuccNode ||
        PointerOperand->stripPointerCasts() == SuccNode) {
      // Case I: Storing a constant integer.
      if (isZeroValue(CurrNode) && llvm::isa<llvm::ConstantInt>(ValueOperand)) {
        PHASAR_LOG_LEVEL(DEBUG, "Case: Storing constant integer.");
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        const auto *CI = llvm::dyn_cast<llvm::ConstantInt>(ValueOperand);
        auto IntConst = CI->getSExtValue();
        return lca::GenConstant{IntConst};
      }
      // Case II: Storing an integer typed value.
      if (CurrNode != SuccNode && ValueOperand->getType()->isIntegerTy()) {
        PHASAR_LOG_LEVEL(DEBUG, "Case: Storing an integer typed value.");
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        return EdgeIdentity<l_t>{};
      }
    }
  }

  // Check load instruction
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    if (Load == SuccNode) {
      PHASAR_LOG_LEVEL(DEBUG, "Case: Loading an integer typed value.");
      PHASAR_LOG_LEVEL(DEBUG, ' ');
      return EdgeIdentity<l_t>{};
    }
  }

  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (Curr == SuccNode && CurrNode != SuccNode &&
      llvm::isa<llvm::BinaryOperator>(Curr)) {
    PHASAR_LOG_LEVEL(DEBUG, "Case: Binary operation.");
    PHASAR_LOG_LEVEL(DEBUG, ' ');
    unsigned OP = Curr->getOpcode();
    auto *Lop = Curr->getOperand(0);
    auto *Rop = Curr->getOperand(1);
    // For non linear constant computation we propagate bottom
    if ((CurrNode == Lop && !llvm::isa<llvm::ConstantInt>(Rop)) ||
        (CurrNode == Rop && !llvm::isa<llvm::ConstantInt>(Lop))) {
      return AllBottom<l_t>{};
    }

    return lca::BinOp(OP, Lop, Rop, CurrNode);
  }

  PHASAR_LOG_LEVEL(DEBUG, "Case: Edge identity.");
  PHASAR_LOG_LEVEL(DEBUG, ' ');
  return EdgeIdentity<l_t>{};
}

EdgeFunction<lca::l_t> IDELinearConstantAnalysis::getCallEdgeFunction(
    n_t CallSite, d_t SrcNode, f_t /*DestinationFunction*/, d_t DestNode) {
  // Case: Passing constant integer as parameter
  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    if (const auto *A = llvm::dyn_cast<llvm::Argument>(DestNode)) {
      const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
      const auto *Actual = CS->getArgOperand(getFunctionArgumentNr(A));
      if (const auto *CI = llvm::dyn_cast<llvm::ConstantInt>(Actual)) {
        auto IntConst = CI->getSExtValue();
        return lca::GenConstant{IntConst};
      }
    }
  }
  return EdgeIdentity<l_t>{};
}

EdgeFunction<lca::l_t> IDELinearConstantAnalysis::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeFunction*/, n_t ExitStmt, d_t ExitNode,
    n_t /*RetSite*/, d_t RetNode) {
  // Case: Returning constant integer
  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    const auto *Return = llvm::cast<llvm::ReturnInst>(ExitStmt);
    auto *ReturnValue = Return->getReturnValue();
    if (auto *CI = llvm::dyn_cast_or_null<llvm::ConstantInt>(ReturnValue)) {
      auto IntConst = CI->getSExtValue();
      return lca::GenConstant{IntConst};
    }
  }
  return EdgeIdentity<l_t>{};
}

EdgeFunction<lca::l_t> IDELinearConstantAnalysis::getCallToRetEdgeFunction(
    n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/, d_t /*RetSiteNode*/,
    llvm::ArrayRef<f_t> /*Callees*/) {
  return EdgeIdentity<l_t>{};
}

EdgeFunction<lca::l_t>
IDELinearConstantAnalysis::getSummaryEdgeFunction(n_t Curr, d_t CurrNode,
                                                  n_t /*Succ*/, d_t SuccNode) {

  if (const auto *BinIntrinsic =
          llvm::dyn_cast<llvm::BinaryOpIntrinsic>(Curr)) {
    auto *Lop = BinIntrinsic->getLHS();
    auto *Rop = BinIntrinsic->getRHS();
    unsigned OP = BinIntrinsic->getBinaryOp();

    // For non linear constant computation we propagate bottom
    if ((CurrNode == Lop && !llvm::isa<llvm::ConstantInt>(Rop)) ||
        (CurrNode == Rop && !llvm::isa<llvm::ConstantInt>(Lop))) {
      return AllBottom<l_t>{};
    }

    if (Curr == SuccNode && CurrNode != SuccNode) {
      return lca::BinOp{OP, Lop, Rop, CurrNode};
    }
  }
  return EdgeIdentity<l_t>{};
}

void IDELinearConstantAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, llvm::raw_ostream &OS) {
  OS << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    OS << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (const auto *F : ICF->getAllFunctions()) {
      std::string FName = getFunctionNameFromIR(F);
      OS << "\nFunction: " << FName << "\n----------"
         << std::string(FName.size(), '-') << '\n';
      for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
        auto Results = SR.resultsAt(Stmt, true);
        stripBottomResults(Results);
        if (!Results.empty()) {
          OS << "At IR statement: " << NToString(Stmt) << '\n';
          for (auto Res : Results) {
            if (!Res.second.isBottom()) {
              OS << "   Fact: " << DToString(Res.first)
                 << "\n  Value: " << LToString(Res.second) << '\n';
            }
          }
          OS << '\n';
        }
      }
      OS << '\n';
    }
  } else {
    auto LcaResults = getLCAResults(SR);
    for (const auto &Entry : LcaResults) {
      OS << "\nFunction: " << Entry.first
         << "\n==========" << std::string(Entry.first.size(), '=') << '\n';
      for (auto FResult : Entry.second) {
        FResult.second.print(OS);
        OS << "--------------------------------------\n\n";
      }
      OS << '\n';
    }
  }
}

void IDELinearConstantAnalysis::stripBottomResults(
    std::unordered_map<d_t, l_t> &Res) {
  for (auto It = Res.begin(); It != Res.end();) {
    if (It->second.isBottom()) {
      It = Res.erase(It);
    } else {
      ++It;
    }
  }
}

IDELinearConstantAnalysis::lca_results_t
IDELinearConstantAnalysis::getLCAResults(SolverResults<n_t, d_t, l_t> SR) {
  std::map<std::string, std::map<unsigned, LCAResult>> AggResults;
  llvm::outs() << "\n==== Computing LCA Results ====\n";
  for (const auto *F : ICF->getAllFunctions()) {
    std::string FName = getFunctionNameFromIR(F);
    llvm::outs() << "\n-- Function: " << FName << " --\n";
    std::map<unsigned, LCAResult> FResults;
    std::set<std::string> AllocatedVars;
    for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
      unsigned Lnr = getLineFromIR(Stmt);
      llvm::outs() << "\nIR : " << NToString(Stmt) << "\nLNR: " << Lnr << '\n';
      // We skip statements with no source code mapping
      if (Lnr == 0) {
        llvm::outs() << "Skipping this stmt!\n";
        continue;
      }
      LCAResult *LcaRes = &FResults[Lnr];
      // Check if it is a new result
      if (LcaRes->SrcNode.empty()) {
        std::string SourceCode = getSrcCodeFromIR(Stmt);
        // Skip results for line containing only closed braces which is the
        // case for functions with void return value
        if (SourceCode == "}") {
          FResults.erase(Lnr);
          continue;
        }
        LcaRes->SrcNode = SourceCode;
        LcaRes->LineNr = Lnr;
      }
      LcaRes->IRTrace.push_back(Stmt);
      if (Stmt->isTerminator() && !ICF->isExitInst(Stmt)) {
        llvm::outs() << "Delete result since stmt is Terminator or Exit!\n";
        FResults.erase(Lnr);
      } else {
        // check results of succ(stmt)
        std::unordered_map<d_t, l_t> Results;
        if (ICF->isExitInst(Stmt)) {
          Results = SR.resultsAt(Stmt, true);
        } else {
          // It's not a terminator inst, hence it has only a single successor
          const auto *Succ = ICF->getSuccsOf(Stmt)[0];
          llvm::outs() << "Succ stmt: " << NToString(Succ) << '\n';
          Results = SR.resultsAt(Succ, true);
        }
        stripBottomResults(Results);
        std::set<std::string> ValidVarsAtStmt;
        for (auto Res : Results) {
          auto VarName = getVarNameFromIR(Res.first);
          llvm::outs() << "  D: " << DToString(Res.first)
                       << " | V: " << LToString(Res.second)
                       << " | Var: " << VarName << '\n';
          if (!VarName.empty()) {
            // Only store/overwrite values of variables from allocas or globals
            // unless there is no value stored for a variable
            if (llvm::isa<llvm::AllocaInst>(Res.first) ||
                llvm::isa<llvm::GlobalVariable>(Res.first)) {
              // lcaRes->variableToValue.find(varName) ==
              // lcaRes->variableToValue.end()) {
              ValidVarsAtStmt.insert(VarName);
              AllocatedVars.insert(VarName);
              LcaRes->VariableToValue[VarName] = Res.second;
            } else if (AllocatedVars.find(VarName) == AllocatedVars.end()) {
              ValidVarsAtStmt.insert(VarName);
              LcaRes->VariableToValue[VarName] = Res.second;
            }
          }
        }
        // remove no longer valid variables at current IR stmt
        for (auto It = LcaRes->VariableToValue.begin();
             It != LcaRes->VariableToValue.end();) {
          if (ValidVarsAtStmt.find(It->first) == ValidVarsAtStmt.end()) {
            llvm::outs() << "Erase var: " << It->first << '\n';
            It = LcaRes->VariableToValue.erase(It);
          } else {
            ++It;
          }
        }
      }
    }
    // delete entries with no result
    for (auto It = FResults.begin(); It != FResults.end();) {
      if (It->second.VariableToValue.empty()) {
        It = FResults.erase(It);
      } else {
        ++It;
      }
    }
    AggResults[FName] = FResults;
  }
  return AggResults;
}

void IDELinearConstantAnalysis::LCAResult::print(llvm::raw_ostream &OS) const {
  OS << "Line " << LineNr << ": " << SrcNode << '\n';
  OS << "Var(s): ";
  for (auto It = VariableToValue.begin(); It != VariableToValue.end(); ++It) {
    if (It != VariableToValue.begin()) {
      OS << ", ";
    }
    OS << It->first << " = " << It->second;
  }
  OS << "\nCorresponding IR Instructions:\n";
  for (const auto *Ir : IRTrace) {
    OS << "  " << llvmIRToString(Ir) << '\n';
  }
}

} // namespace psr
