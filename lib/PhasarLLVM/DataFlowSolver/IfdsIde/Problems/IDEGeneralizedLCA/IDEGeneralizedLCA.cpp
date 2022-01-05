/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <sstream>

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/BinaryEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCalleeFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCallerFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/TypecastEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSToIDETabulationProblem.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/Logger.h"

namespace psr {

template <typename Fn, typename = std::enable_if_t<
                           std::is_invocable_v<Fn, IDEGeneralizedLCA::d_t>>>
inline std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>> flow(Fn Func) {
  return makeLambdaFlow<IDEGeneralizedLCA::d_t>(std::forward<Fn>(Func));
}

IDEGeneralizedLCA::IDEGeneralizedLCA(
    const ProjectIRDB *IRDB,
    const TypeHierarchy<const llvm::StructType *, const llvm::Function *> *TH,
    const LLVMBasedICFG *ICF,
    PointsToInfo<const llvm::Value *, const llvm::Instruction *> *PT,
    std::set<std::string> EntryPoints, size_t MaxSetSize)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)),
      MaxSetSize(MaxSetSize) {
  this->ZeroValue = IDEGeneralizedLCA::createZeroValue();
}

// flow functions
std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>>
IDEGeneralizedLCA::getNormalFlowFunction(IDEGeneralizedLCA::n_t Curr,
                                         IDEGeneralizedLCA::n_t /*Succ*/) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    const auto *PointerOp = Store->getPointerOperand();
    const auto *ValueOp = Store->getValueOperand();
    if (isConstant(ValueOp)) {
      // std::cout << "==> constant store" << std::endl;
      return flow([=](IDEGeneralizedLCA::d_t Source)
                      -> std::set<IDEGeneralizedLCA::d_t> {
        // std::cout << "##> normal flow for: " << llvmIRToString(curr)
        //          << " with " << llvmIRToString(source) << std::endl;
        if (Source == PointerOp) {
          return {};
        }
        if (this->isZeroValue(Source)) {
          return {PointerOp};
        }
        return {Source};
      });
    }
    return flow(
        [=](IDEGeneralizedLCA::d_t Source) -> std::set<IDEGeneralizedLCA::d_t> {
          if (Source == PointerOp) {
            return {};
          }
          if (Source == ValueOp) {
            return {PointerOp, ValueOp};
          }
          return {Source};
        });
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return flow(
        [=](IDEGeneralizedLCA::d_t Source) -> std::set<IDEGeneralizedLCA::d_t> {
          // std::cout << "LOAD " << llvmIRToString(curr) << std::endl;
          // std::cout << "\twith " << llvmIRToString(source) << " ==> ";
          if (Source == Load->getPointerOperand()) {
            // std::cout << "GEN" << std::endl;
            return {Source, Load};
          }
          // std::cout << "ID" << std::endl;
          return {Source};
        });
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return flow(
        [=](IDEGeneralizedLCA::d_t Source) -> std::set<IDEGeneralizedLCA::d_t> {
          if (Source == Gep->getPointerOperand()) {
            return {Source, Gep};
          }
          return {Source};
        });
  }
  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Curr);
      Cast &&
      (Cast->getSrcTy()->isIntegerTy() ||
       Cast->getSrcTy()->isFloatingPointTy()) &&
      (Cast->getDestTy()->isIntegerTy() ||
       Cast->getDestTy()->isFloatingPointTy())) {
    return flow(
        [=](IDEGeneralizedLCA::d_t Source) -> std::set<IDEGeneralizedLCA::d_t> {
          if (Source == Cast->getOperand(0)) {
            return {Source, Cast};
          }
          return {Source};
        });
  }
  if (llvm::isa<llvm::BinaryOperator>(Curr)) {
    const auto *Lhs = Curr->getOperand(0);
    const auto *Rhs = Curr->getOperand(1);
    bool LeftConst = isConstant(Lhs);
    bool RightConst = isConstant(Rhs);
    bool BothConst = LeftConst && RightConst;
    bool NoneConst = !LeftConst && !RightConst;

    return flow(
        [=](IDEGeneralizedLCA::d_t Source) -> std::set<IDEGeneralizedLCA::d_t> {
          if (Source == Lhs || Source == Rhs ||
              ((BothConst || NoneConst) && isZeroValue(Source))) {
            return {Source, Curr};
          }
          return {Source};
        });
  } /*else if (llvm::isa<llvm::UnaryOperator>(curr)) {
    auto op = curr->getOperand(0);
    return flow([=](IDEGeneralizedLCA::d_t source)
                    -> std::set<IDEGeneralizedLCA::d_t> {
      if (source == op)
        return {source, curr};
      else
        return {source};
    });
  }
*/
  return Identity<IDEGeneralizedLCA::d_t>::getInstance();
}

std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>>
IDEGeneralizedLCA::getCallFlowFunction(IDEGeneralizedLCA::n_t CallStmt,
                                       IDEGeneralizedLCA::f_t DestMthd) {
  assert(llvm::isa<llvm::CallBase>(CallStmt));
  if (isStringConstructor(DestMthd)) {
    // kill all data-flow facts at calls to string constructors
    return KillAll<IDEGeneralizedLCA::d_t>::getInstance();
  }
  return std::make_shared<MapFactsToCalleeFlowFunction>(
      llvm::cast<llvm::CallBase>(CallStmt), DestMthd);
}

std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>>
IDEGeneralizedLCA::getRetFlowFunction(IDEGeneralizedLCA::n_t CallSite,
                                      IDEGeneralizedLCA::f_t CalleeMthd,
                                      IDEGeneralizedLCA::n_t ExitStmt,
                                      IDEGeneralizedLCA::n_t /*RetSite*/) {
  assert(llvm::isa<llvm::CallBase>(CallSite));
  // std::cout << "Ret flow: " << llvmIRToString(ExitStmt) << std::endl;
  /*return std::make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](const llvm::Value *v) -> bool {
        return v && v->getType()->isPointerTy();
      });*/
  return std::make_shared<MapFactsToCallerFlowFunction>(
      llvm::cast<llvm::CallBase>(CallSite), ExitStmt, CalleeMthd);
}

std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>>
IDEGeneralizedLCA::getCallToRetFlowFunction(IDEGeneralizedLCA::n_t CallSite,
                                            IDEGeneralizedLCA::n_t /*RetSite*/,
                                            std::set<f_t> /*Callees*/) {
  // std::cout << "CTR flow: " << llvmIRToString(CallSite) << std::endl;
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    // check for ctor and then demangle function name and check for
    // std::basic_string
    if (isStringConstructor(CS->getCalledFunction())) {
      // found std::string ctor
      return std::make_shared<Gen<IDEGeneralizedLCA::d_t>>(CS->getArgOperand(0),
                                                           getZeroValue());
    }
    // return flow([Call](IDEGeneralizedLCA::d_t Source)
    //                 -> std::set<IDEGeneralizedLCA::d_t> {
    //   // std::cout << "In getCallToRetFlowFunction\n";
    //   // std::cout << llvmIRToString(Source) << '\n';
    //   if (Source->getType()->isPointerTy()) {
    //     for (auto &Arg : Call->arg_operands()) {
    //       if (Arg.get() == Source) {
    //         return {};
    //       }
    //     }
    //   }
    //   return {Source};
    // });
  }
  return Identity<d_t>::getInstance();
}

std::shared_ptr<FlowFunction<IDEGeneralizedLCA::d_t>>
IDEGeneralizedLCA::getSummaryFlowFunction(IDEGeneralizedLCA::n_t /*CallStmt*/,
                                          IDEGeneralizedLCA::f_t /*DestMthd*/) {
  // std::cout << "Summary flow: " << llvmIRToString(callStmt) <<
  // std::endl;
  return nullptr;
}

InitialSeeds<IDEGeneralizedLCA::n_t, IDEGeneralizedLCA::d_t,
             IDEGeneralizedLCA::l_t>
IDEGeneralizedLCA::initialSeeds() {
  InitialSeeds<IDEGeneralizedLCA::n_t, IDEGeneralizedLCA::d_t,
               IDEGeneralizedLCA::l_t>
      Seeds;
  // For now, out only entrypoint is main:
  std::vector<std::string> EntryPoints = {"main"};
  for (auto &EntryPoint : EntryPoints) {
    std::set<IDEGeneralizedLCA::d_t> Globals;
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), bottomElement());
    for (const auto &G :
         IRDB->getModuleDefiningFunction(EntryPoint)->globals()) {
      if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
        if (GV->hasInitializer()) {
          if (llvm::isa<llvm::ConstantInt>(GV->getInitializer()) ||
              llvm::isa<llvm::ConstantDataArray>(GV->getInitializer())) {
            Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(), GV,
                          bottomElement());
          }
        }
      }
    }
  }
  return Seeds;
}

IDEGeneralizedLCA::d_t IDEGeneralizedLCA::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool IDEGeneralizedLCA::isZeroValue(IDEGeneralizedLCA::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

// edge functions
std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::getNormalEdgeFunction(IDEGeneralizedLCA::n_t Curr,
                                         IDEGeneralizedLCA::d_t CurrNode,
                                         IDEGeneralizedLCA::n_t Succ,
                                         IDEGeneralizedLCA::d_t SuccNode) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IDEGeneralizedLCA::getNormalEdgeFunction()");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "(N) Curr Inst : " << IDEGeneralizedLCA::NtoString(Curr));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "(D) Curr Node :   "
                << IDEGeneralizedLCA::DtoString(CurrNode));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "(N) Succ Inst : " << IDEGeneralizedLCA::NtoString(Succ));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "(D) Succ Node :   "
                << IDEGeneralizedLCA::DtoString(SuccNode));
  // Initialize global variables at entry point
  if (!isZeroValue(CurrNode) && ICF->isStartPoint(Curr) &&
      isEntryPoint(ICF->getFunctionOf(Curr)->getName().str()) &&
      llvm::isa<llvm::GlobalVariable>(CurrNode) && CurrNode == SuccNode) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Case: Intialize global variable at entry point.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    const auto *GV = llvm::cast<llvm::GlobalVariable>(CurrNode);
    if (GV->getLinkage() != llvm::GlobalValue::LinkageTypes::
                                CommonLinkage) { // clang uses common linkage
                                                 // for uninitialized globals
      if (const auto *CI =
              llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer())) {
        auto IntConst = CI->getValue();
        return std::make_shared<GenConstant>(
            l_t({EdgeValue(std::move(IntConst))}), MaxSetSize);
      }
      if (const auto *CF =
              llvm::dyn_cast<llvm::ConstantFP>(GV->getInitializer())) {
        auto FPConst = CF->getValueAPF();
        return std::make_shared<GenConstant>(
            l_t({EdgeValue(std::move(FPConst))}), MaxSetSize);
      }
      if (const auto *CS =
              llvm::dyn_cast<llvm::ConstantDataArray>(GV->getInitializer())) {
        auto StringConst = CS->getAsCString();
        return std::make_shared<GenConstant>(
            l_t({EdgeValue(StringConst.str())}), MaxSetSize);
      }
    }
  }

  // All_Bottom for zero value
  if (isZeroValue(CurrNode) && isZeroValue(SuccNode)) {
    static auto AllBot = std::make_shared<AllBottom<l_t>>(bottomElement());
    return AllBot;
  }
  // Check store instruction
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    IDEGeneralizedLCA::d_t PointerOperand = Store->getPointerOperand();
    IDEGeneralizedLCA::d_t ValueOperand = Store->getValueOperand();
    /*if (auto cnstFP = llvm::dyn_cast<llvm::ConstantFP>(valueOperand)) {
      llvm::errs() << "Value Operand: " << *cnstFP << "\n";
      llvm::errs() << "ValueOperand as APF: ";
      cnstFP->getValueAPF().print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "Value operand as double: "
                   << cnstFP->getValueAPF().convertToDouble() << "\n";
    }*/
    if (PointerOperand == SuccNode) {
      // Case I: Storing a constant value.
      if (isZeroValue(CurrNode) && isConstant(ValueOperand)) {
        EdgeValue Ev(ValueOperand);
        return std::make_shared<GenConstant>(l_t({Ev}), MaxSetSize);
      }
      // Case II: Storing an integer typed value.
      /*if (currNode != succNode && valueOperand->getType()->isIntegerTy()) {
        return IdentityEdgeFunction::getInstance(maxSetSize);
        // return std::make_shared<DebugIdentityEdgeFunction>(curr, succ,
        //                                                  maxSetSize);
      }*/
    }
  }

  // Check load instruction
  /*if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load == succNode) {
      // std::cout << "LOAD " << llvmIRToString(curr) << " TO "
      //           << llvmIRToString(succ) << std::endl;
      // return EdgeIdentity<l_t>::getInstance();
      // return std::make_shared<DebugIdentityEdgeFunction>(curr, succ,
      //                                                  maxSetSize);
      return IdentityEdgeFunction::getInstance(maxSetSize);
    }
  }*/
  // binary operators
  if (const auto *BinOp = llvm::dyn_cast<llvm::BinaryOperator>(Curr);
      BinOp && Curr == SuccNode) {
    // BinaryEdgeFunction(op, cnst, leftConst, MaxSize)
    if (isConstant(Curr->getOperand(0))) {
      EdgeValue Lcnst(Curr->getOperand(0));
      if (isConstant(Curr->getOperand(1)) && isZeroValue(CurrNode)) {
        // Both const
        EdgeValue Rcnst(Curr->getOperand(1));
        auto Ret = // join({lcnst}, {rcnst});
            performBinOp(BinOp->getOpcode(), {Lcnst}, {Rcnst}, MaxSetSize);
        return std::make_shared<GenConstant>(Ret, MaxSetSize);
      }
      // only lhs const
      return std::make_shared<BinaryEdgeFunction>(
          BinOp->getOpcode(), l_t({Lcnst}), true, MaxSetSize);
    }
    if (!isConstant(Curr->getOperand(1))) {
      // none const
      return std::make_shared<GenConstant>(bottomElement(), MaxSetSize);
    }
    // only rhs const
    EdgeValue Rcnst(Curr->getOperand(1));
    return std::make_shared<BinaryEdgeFunction>(
        BinOp->getOpcode(), l_t({Rcnst}), false, MaxSetSize);
  }
  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Curr);
      Cast && Curr == SuccNode) {
    if (Cast->getDestTy()->isIntegerTy()) {
      const auto *DestTy = llvm::cast<llvm::IntegerType>(Cast->getDestTy());

      return std::make_shared<TypecastEdgeFunction>(
          DestTy->getBitWidth(), EdgeValue::Integer, MaxSetSize);
    }
    if (Cast->getDestTy()->isFloatingPointTy()) {
      auto Bits = Cast->getDestTy()->isFloatTy() ? 32 : 64;

      return std::make_shared<TypecastEdgeFunction>(
          Bits, EdgeValue::FloatingPoint, MaxSetSize);
    }
  }
  // std::cout << "FallThrough: identity edge fn" << std::endl;
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::getCallEdgeFunction(IDEGeneralizedLCA::n_t CallStmt,
                                       IDEGeneralizedLCA::d_t SrcNode,
                                       IDEGeneralizedLCA::f_t DestinationMethod,
                                       IDEGeneralizedLCA::d_t DestNode) {
  const auto *CallSite = llvm::cast<llvm::CallBase>(CallStmt);
  if (isZeroValue(SrcNode)) {
    auto Len = std::min<size_t>(CallSite->getNumArgOperands(),
                                DestinationMethod->arg_size());
    for (size_t I = 0; I < Len; ++I) {
      const auto *FormalArg = getNthFunctionArgument(DestinationMethod, I);
      if (DestNode == FormalArg) {
        const auto *ActualArg = CallSite->getArgOperand(I);
        // if (isConstant(actualArg))  // -> always const, since srcNode is zero
        return std::make_shared<GenConstant>(l_t({EdgeValue(ActualArg)}),
                                             MaxSetSize);
      }
    }
  }
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::getReturnEdgeFunction(
    IDEGeneralizedLCA::n_t /*CallSite*/,
    IDEGeneralizedLCA::f_t /*CalleeMethod*/, IDEGeneralizedLCA::n_t ExitStmt,
    IDEGeneralizedLCA::d_t ExitNode, IDEGeneralizedLCA::n_t /*RetSite*/,
    IDEGeneralizedLCA::d_t /*RetNode*/) {
  if (isZeroValue(ExitNode)) {
    if (const auto *RetStmt = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)) {
      if (RetStmt->getReturnValue() && isConstant(RetStmt->getReturnValue())) {
        // std::cout << "Constant return value: "
        //          << llvmIRToShortString(exitStmt) << std::endl;
        return std::make_shared<GenConstant>(
            l_t({EdgeValue(RetStmt->getReturnValue())}), MaxSetSize);
      }
    }
  }
  // std::cout << "Return identity: " << llvmIRToShortString(exitStmt)
  //          << std::endl;
  // return edge-identity
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::getCallToRetEdgeFunction(
    IDEGeneralizedLCA::n_t CallSite, IDEGeneralizedLCA::d_t CallNode,
    IDEGeneralizedLCA::n_t /*RetSite*/, IDEGeneralizedLCA::d_t RetSiteNode,
    std::set<IDEGeneralizedLCA::f_t> /*Callees*/) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

  // check for ctor and then demangle function name and check for
  // std::basic_string
  if (isStringConstructor(CS->getCalledFunction())) {
    // found correct place and time
    if (CallNode == getZeroValue() && RetSiteNode == CS->getArgOperand(0)) {
      // find string literal that is used to initialize the string
      if (auto *User = llvm::dyn_cast<llvm::User>(CS->getArgOperand(1))) {
        if (auto *GV =
                llvm::dyn_cast<llvm::GlobalVariable>(User->getOperand(0))) {
          if (!GV->hasInitializer()) {
            // in this case we don't know the initial value statically
            // return ALLBOTTOM;
            return std::make_shared<AllBottom<l_t>>(bottomElement());
          }
          if (auto *CDA = llvm::dyn_cast<llvm::ConstantDataArray>(
                  GV->getInitializer())) {
            if (CDA->isCString()) {
              // here we statically know the string literal the std::string is
              // initialized with
              return std::make_shared<GenConstant>(
                  l_t({EdgeValue(CDA->getAsCString().str())}), MaxSetSize);
            }
          }
        }
      }
    }
  }

  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::getSummaryEdgeFunction(
    IDEGeneralizedLCA::n_t /*CallStmt*/, IDEGeneralizedLCA::d_t /*CallNode*/,
    IDEGeneralizedLCA::n_t /*RetSite*/,
    IDEGeneralizedLCA::d_t /*RetSiteNode*/) {
  // return edge-identity
  return EdgeIdentity<l_t>::getInstance();
}

IDEGeneralizedLCA::l_t IDEGeneralizedLCA::topElement() { return {{}}; }

IDEGeneralizedLCA::l_t IDEGeneralizedLCA::bottomElement() {
  return l_t({EdgeValue::TopValue});
}

IDEGeneralizedLCA::l_t IDEGeneralizedLCA::join(IDEGeneralizedLCA::l_t Lhs,
                                               IDEGeneralizedLCA::l_t Rhs) {
  // sets are passed by value
  return psr::join(Lhs, Rhs, MaxSetSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IDEGeneralizedLCA::allTopFunction() {
  static std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> AlltopFn =
      std::make_shared<AllTop<l_t>>(topElement());
  return AlltopFn;
}

void IDEGeneralizedLCA::printNode(std::ostream &Os,
                                  IDEGeneralizedLCA::n_t Stmt) const {
  Os << llvmIRToString(Stmt);
}

void IDEGeneralizedLCA::printDataFlowFact(std::ostream &Os,
                                          IDEGeneralizedLCA::d_t Fact) const {
  assert(Fact && "Invalid dataflow fact");
  Os << llvmIRToString(Fact);
}

void IDEGeneralizedLCA::printFunction(std::ostream &Os,
                                      IDEGeneralizedLCA::f_t Func) const {
  Os << Func->getName().str();
}

void IDEGeneralizedLCA::printEdgeFact(std::ostream &Os,
                                      IDEGeneralizedLCA::l_t L) const {
  Os << L;
}

/*void IDEGeneralizedLCA::printIDEReport(
    std::ostream &os,
    SolverResults<IDEGeneralizedLCA::n_t,
                       IDEGeneralizedLCA::d_t,
                       IDEGeneralizedLCA::l_t> &SR) {

  os << "\n======= LCP RESULTS =======\n";
  for (auto f : icfg.getAllMethods()) {
    os << llvmFunctionToSrc(f) << '\n';
    for (auto exit : icfg.getExitPointsOf(f)) {
      auto results = SR.resultsAt(exit, true);
      if (results.empty()) {
        os << "\nNo results available!\n";
      } else {
        for (auto res : results) {
          if (!llvm::isa<llvm::LoadInst>(res.first)) {
            os << "\nValue: " << VtoString(res.second)
               << "\nIR  : " << DtoString(res.first) << '\n'
               << llvmValueToSrc(res.first, false) << "\n";
          }
        }
      }
    }
    os << "----------------\n";
  }
}*/

void IDEGeneralizedLCA::emitTextReport(
    const SolverResults<IDEGeneralizedLCA::n_t, IDEGeneralizedLCA::d_t,
                        IDEGeneralizedLCA::l_t> &SR,
    std::ostream &Os) {

  Os << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    Os << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (const auto *F : ICF->getAllFunctions()) {
      std::string FName = getFunctionNameFromIR(F);
      Os << "\nFunction: " << FName << "\n----------"
         << std::string(FName.size(), '-') << '\n';
      for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
        auto Results = SR.resultsAt(Stmt, true);
        stripBottomResults(Results);
        if (!Results.empty()) {
          Os << "At IR statement: " << NtoString(Stmt) << '\n';
          for (const auto &Res : Results) {
            if (Res.second != bottomElement()) {
              Os << "   Fact: " << DtoString(Res.first)
                 << "\n  Value: " << VtoString(Res.second) << '\n';
            }
          }
          Os << '\n';
        }
      }
      Os << '\n';
    }
  } else {
    auto LcaResults = getLCAResults(SR);
    for (const auto &Entry : LcaResults) {
      Os << "\nFunction: " << Entry.first
         << "\n==========" << std::string(Entry.first.size(), '=') << '\n';
      for (auto FResult : Entry.second) {
        FResult.second.print(Os);
        Os << "--------------------------------------\n\n";
      }
      Os << '\n';
    }
  }
}

void IDEGeneralizedLCA::stripBottomResults(
    std::unordered_map<IDEGeneralizedLCA::d_t, IDEGeneralizedLCA::l_t> &Res) {
  for (auto It = Res.begin(); It != Res.end();) {
    if (It->second == bottomElement()) {
      It = Res.erase(It);
    } else {
      ++It;
    }
  }
}

IDEGeneralizedLCA::lca_results_t IDEGeneralizedLCA::getLCAResults(
    SolverResults<IDEGeneralizedLCA::n_t, IDEGeneralizedLCA::d_t,
                  IDEGeneralizedLCA::l_t>
        SR) {
  std::map<std::string, std::map<unsigned, LCAResult>> AggResults;
  std::cout << "\n==== Computing LCA Results ====\n";
  for (const auto *F : ICF->getAllFunctions()) {
    std::string FName = getFunctionNameFromIR(F);
    std::cout << "\n-- Function: " << FName << " --\n";
    std::map<unsigned, LCAResult> FResults;
    std::set<std::string> AllocatedVars;
    for (const auto *Stmt : ICF->getAllInstructionsOf(F)) {
      unsigned Lnr = getLineFromIR(Stmt);
      std::cout << "\nIR : " << NtoString(Stmt) << "\nLNR: " << Lnr << '\n';
      // We skip statements with no source code mapping
      if (Lnr == 0) {
        std::cout << "Skipping this stmt!\n";
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
        LcaRes->LineNo = Lnr;
      }
      LcaRes->IRTrace.push_back(Stmt);
      if (Stmt->isTerminator() && !ICF->isExitInst(Stmt)) {
        std::cout << "Delete result since stmt is Terminator or Exit!\n";
        FResults.erase(Lnr);
      } else {
        // check results of succ(stmt)
        std::unordered_map<d_t, l_t> Results;
        if (ICF->isExitInst(Stmt)) {
          Results = SR.resultsAt(Stmt, true);
        } else {
          // It's not a terminator inst, hence it has only a single successor
          const auto *Succ = ICF->getSuccsOf(Stmt)[0];
          std::cout << "Succ stmt: " << NtoString(Succ) << '\n';
          Results = SR.resultsAt(Succ, true);
        }
        // stripBottomResults(results);
        std::set<std::string> ValidVarsAtStmt;
        for (const auto &Res : Results) {
          auto VarName = getVarNameFromIR(Res.first);
          std::cout << "  D: " << DtoString(Res.first)
                    << " | V: " << VtoString(Res.second)
                    << " | Var: " << VarName << '\n';
          if (!VarName.empty()) {
            // Only store/overwrite values of variables from allocas or
            // globals unless there is no value stored for a variable
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
            std::cout << "Erase var: " << It->first << '\n';
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

void IDEGeneralizedLCA::LCAResult::print(std::ostream &Os) {
  Os << "Line " << LineNo << ": " << SrcNode << '\n';
  Os << "Var(s): ";
  for (auto It = VariableToValue.begin(); It != VariableToValue.end(); ++It) {
    if (It != VariableToValue.begin()) {
      Os << ", ";
    }
    Os << It->first << " = " << It->second;
  }
  Os << "\nCorresponding IR Instructions:\n";
  for (const auto *Ir : IRTrace) {
    Os << "  " << llvmIRToString(Ir) << '\n';
  }
}

bool IDEGeneralizedLCA::isEntryPoint(const std::string &Name) const {
  // For now, the only entrypoint is main
  return Name == "main";
}

template <typename V> std::string IDEGeneralizedLCA::VtoString(V Val) {
  std::stringstream Ss;
  Ss << Val;
  return Ss.str();
}

bool IDEGeneralizedLCA::isStringConstructor(const llvm::Function *F) {
  return (ICF->getSpecialMemberFunctionType(F) ==
              SpecialMemberFunctionType::Constructor &&
          llvm::demangle(F->getName().str())
                  .find("::allocator<char> >::basic_string") !=
              std::string::npos);
}

} // namespace psr
