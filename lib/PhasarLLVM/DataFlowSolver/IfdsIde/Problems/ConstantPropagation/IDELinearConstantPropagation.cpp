// TODO:
// - add authors
// - run clang-tidy and fix identifier naming
// - rename file to make clear that is a generalized LCA

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/LambdaFlow.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/BinaryEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/DebugIdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/IdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/MapFactsToCalleeFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/MapFactsToCallerFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/TypecastEdgeFunction.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;
using namespace LCUtils;

namespace psr {

template <typename V> std::string VtoString(V v) {
  std::stringstream ss;
  ss << v;
  return ss.str();
}

inline std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
flow(std::function<std::set<IDELinearConstantPropagation::d_t>(
         IDELinearConstantPropagation::d_t)>
         fn) {
  return std::make_shared<LambdaFlow<IDELinearConstantPropagation::d_t>>(fn);
}

IDELinearConstantPropagation::IDELinearConstantPropagation(
    const ProjectIRDB *IRDB,
    const TypeHierarchy<const llvm::StructType *, const llvm::Function *> *TH,
    const LLVMBasedICFG *ICF,
    const PointsToInfo<const llvm::Value *, const llvm::Instruction *> *PT,
    std::set<std::string> EntryPoints, size_t MaxSetSize)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints),
      maxSetSize(MaxSetSize) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}
// flow functions
std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::getNormalFlowFunction(
    IDELinearConstantPropagation::n_t curr,
    IDELinearConstantPropagation::n_t succ) {
  // std::cout << "## normal flow for: " << psr::llvmIRToString(curr) <<
  // std::endl;
  if (auto store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    auto pointerOp = store->getPointerOperand();
    auto valueOp = store->getValueOperand();
    if (isConstant(valueOp)) {
      // std::cout << "==> constant store" << std::endl;
      return flow([=](IDELinearConstantPropagation::d_t source)
                      -> std::set<IDELinearConstantPropagation::d_t> {
        // std::cout << "##> normal flow for: " << psr::llvmIRToString(curr)
        //          << " with " << psr::llvmIRToString(source) << std::endl;
        if (source == pointerOp)
          return {};
        else if (this->isZeroValue(source))
          return {pointerOp};
        else
          return {source};
      });
    } else {
      return flow([=](IDELinearConstantPropagation::d_t source)
                      -> std::set<IDELinearConstantPropagation::d_t> {
        // std::cout << "BLA" << std::endl;
        if (source == pointerOp)
          return {};
        else if (source == valueOp)
          return {pointerOp, valueOp};
        else
          return {source};
      });
    }
  } else if (auto load = llvm::dyn_cast<llvm::LoadInst>(curr)) {

    return flow([=](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      // std::cout << "LOAD " << psr::llvmIRToString(curr) << std::endl;
      // std::cout << "\twith " << psr::llvmIRToString(source) << " ==> ";
      if (source == load->getPointerOperand()) {
        // std::cout << "GEN" << std::endl;
        return {source, load};
      } else {
        // std::cout << "ID" << std::endl;
        return {source};
      }
    });
  } else if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(curr)) {
    return flow([=](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      if (source == gep->getPointerOperand())
        return {source, gep};
      else
        return {source};
    });

  } else if (auto cast = llvm::dyn_cast<llvm::CastInst>(curr);
             cast &&
             (cast->getSrcTy()->isIntegerTy() ||
              cast->getSrcTy()->isFloatingPointTy()) &&
             (cast->getDestTy()->isIntegerTy() ||
              cast->getDestTy()->isFloatingPointTy())) {
    return flow([=](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      if (source == cast->getOperand(0))
        return {source, cast};
      else
        return {source};
    });
  } else if (llvm::isa<llvm::BinaryOperator>(curr)) {
    auto lhs = curr->getOperand(0);
    auto rhs = curr->getOperand(1);
    // if (isConstant(lhs) || isConstant(rhs)) {
    bool leftConst = isConstant(lhs);
    bool rightConst = isConstant(rhs);
    bool bothConst = leftConst && rightConst;
    bool noneConst = !leftConst && !rightConst;
    return flow([=](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      // std::cout << "BLUBB" << std::endl;
      if (source == lhs || source == rhs ||
          ((bothConst || noneConst) && isZeroValue(source)))
        return {source, curr};
      else
        return {source};
    });
    //}
  } /*else if (llvm::isa<llvm::UnaryOperator>(curr)) {
    auto op = curr->getOperand(0);
    return flow([=](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      // std::cout << "BLIBLA" << std::endl;
      if (source == op)
        return {source, curr};
      else
        return {source};
    });
  }
*/
  return Identity<IDELinearConstantPropagation::d_t>::getInstance();
}
std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::getCallFlowFunction(
    IDELinearConstantPropagation::n_t callStmt,
    IDELinearConstantPropagation::m_t destMthd) {
  // std::cout << "Call flow: " << psr::llvmIRToString(callStmt) << std::endl;
  // return std::make_shared<psr::MapFactsToCallee>(
  //    llvm::ImmutableCallSite(callStmt), destMthd);
  return std::make_shared<MapFactsToCalleeFlowFunction>(
      llvm::ImmutableCallSite(callStmt), destMthd);
}

std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::getRetFlowFunction(
    IDELinearConstantPropagation::n_t callSite,
    IDELinearConstantPropagation::m_t calleeMthd,
    IDELinearConstantPropagation::n_t exitStmt,
    IDELinearConstantPropagation::n_t retSite) {
  // std::cout << "Ret flow: " << psr::llvmIRToString(exitStmt) << std::endl;
  /*return std::make_shared<psr::MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](const llvm::Value *v) -> bool {
        return v && v->getType()->isPointerTy();
      });*/
  return std::make_shared<MapFactsToCallerFlowFunction>(
      llvm::ImmutableCallSite(callSite), exitStmt, calleeMthd);
}
std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::getCallToRetFlowFunction(
    IDELinearConstantPropagation::n_t callSite,
    IDELinearConstantPropagation::n_t retSite, std::set<m_t> callees) {
  // std::cout << "CTR flow: " << psr::llvmIRToString(callSite) << std::endl;
  if (auto call = llvm::dyn_cast<llvm::CallBase>(callSite)) {

    return flow([call](IDELinearConstantPropagation::d_t source)
                    -> std::set<IDELinearConstantPropagation::d_t> {
      if (source->getType()->isPointerTy()) {
        for (auto &arg : call->arg_operands()) {
          if (arg.get() == source)
            return {};
        }
      }
      return {source};
    });
  } else
    return Identity<d_t>::getInstance();
}
std::shared_ptr<FlowFunction<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::getSummaryFlowFunction(
    IDELinearConstantPropagation::n_t callStmt,
    IDELinearConstantPropagation::m_t destMthd) {
  // std::cout << "Summary flow: " << psr::llvmIRToString(callStmt) <<
  // std::endl;
  return nullptr;
}
std::map<IDELinearConstantPropagation::n_t,
         std::set<IDELinearConstantPropagation::d_t>>
IDELinearConstantPropagation::initialSeeds() {
  std::map<IDELinearConstantPropagation::n_t,
           std::set<IDELinearConstantPropagation::d_t>>
      SeedMap;
  // For now, out only entrypoint is main:
  std::vector<std::string> EntryPoints = {"main"};
  for (auto &EntryPoint : EntryPoints) {
    std::set<IDELinearConstantPropagation::d_t> Globals;
    for (const auto &G :
         IRDB->getModuleDefiningFunction(EntryPoint)->globals()) {
      if (auto GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
        if (GV->hasInitializer()) {
          if (llvm::isa<llvm::ConstantInt>(GV->getInitializer()) ||
              llvm::isa<llvm::ConstantDataArray>(GV->getInitializer()))
            Globals.insert(GV);
        }
      }
    }

    Globals.insert(ZeroValue);
    if (!Globals.empty()) {
      SeedMap.insert(std::make_pair(
          &ICF->getFunction(EntryPoint)->front().front(), Globals));
    }
  }
  // SeedMap.insert(
  //    make_pair(&icfg.getMethod("main")->front().front(),
  //              set<IDELinearConstantPropagation::d_t>({zeroValue()})));
  return SeedMap;
}

IDELinearConstantPropagation::d_t
IDELinearConstantPropagation::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantPropagation::isZeroValue(
    IDELinearConstantPropagation::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// edge functions
std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::getNormalEdgeFunction(
    IDELinearConstantPropagation::n_t curr,
    IDELinearConstantPropagation::d_t currNode,
    IDELinearConstantPropagation::n_t succ,
    IDELinearConstantPropagation::d_t succNode) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IDELinearConstantPropagation::getNormalEdgeFunction()");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(N) Curr Inst : "
                << IDELinearConstantPropagation::NtoString(curr));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(D) Curr Node :   "
                << IDELinearConstantPropagation::DtoString(currNode));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(N) Succ Inst : "
                << IDELinearConstantPropagation::NtoString(succ));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(D) Succ Node :   "
                << IDELinearConstantPropagation::DtoString(succNode));
  //  normal edge fn

  // Initialize global variables at entry point
  if (!isZeroValue(currNode) && ICF->isStartPoint(curr) &&
      isEntryPoint(ICF->getFunctionOf(curr)->getName().str()) &&
      llvm::isa<llvm::GlobalVariable>(currNode) && currNode == succNode) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Case: Intialize global variable at entry point.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    auto GV = llvm::cast<llvm::GlobalVariable>(currNode);
    if (GV->getLinkage() != llvm::GlobalValue::LinkageTypes::
                                CommonLinkage) { // clang uses common linkage
                                                 // for uninitialized globals

      if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(GV->getInitializer())) {

        auto IntConst = CI->getValue();
        return std::make_shared<GenConstant>(
            v_t({EdgeValue(std::move(IntConst))}), maxSetSize);
      } else if (auto CF =
                     llvm::dyn_cast<llvm::ConstantFP>(GV->getInitializer())) {
        auto FPConst = CF->getValueAPF();
        return std::make_shared<GenConstant>(
            v_t({EdgeValue(std::move(FPConst))}), maxSetSize);
      } else if (auto CS = llvm::dyn_cast<llvm::ConstantDataArray>(
                     GV->getInitializer())) {
        auto StringConst = CS->getAsCString();
        return std::make_shared<GenConstant>(
            v_t({EdgeValue(StringConst.str())}), maxSetSize);
      }
    }
  }

  // All_Bottom for zero value
  if (isZeroValue(currNode) && isZeroValue(succNode)) {
    static auto allBottom = std::make_shared<AllBottom<v_t>>(bottomElement());
    return allBottom;
  }
  // Check store instruction
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    IDELinearConstantPropagation::d_t pointerOperand =
        Store->getPointerOperand();
    IDELinearConstantPropagation::d_t valueOperand = Store->getValueOperand();
    /*if (auto cnstFP = llvm::dyn_cast<llvm::ConstantFP>(valueOperand)) {
      llvm::errs() << "Value Operand: " << *cnstFP << "\n";
      llvm::errs() << "ValueOperand as APF: ";
      cnstFP->getValueAPF().print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "Value operand as double: "
                   << cnstFP->getValueAPF().convertToDouble() << "\n";
    }*/
    if (pointerOperand == succNode) {
      // Case I: Storing a constant value.
      if (isZeroValue(currNode) && isConstant(valueOperand)) {
        EdgeValue ev(valueOperand);
        return std::make_shared<GenConstant>(v_t({ev}), maxSetSize);
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
      // std::cout << "LOAD " << psr::llvmIRToString(curr) << " TO "
      //           << psr::llvmIRToString(succ) << std::endl;
      // return psr::EdgeIdentity<v_t>::getInstance();
      // return std::make_shared<DebugIdentityEdgeFunction>(curr, succ,
      //                                                  maxSetSize);
      return IdentityEdgeFunction::getInstance(maxSetSize);
    }
  }*/
  // binary operators
  if (auto binOp = llvm::dyn_cast<llvm::BinaryOperator>(curr);
      binOp && curr == succNode) {

    // BinaryEdgeFunction(op, cnst, leftConst, maxSize)
    if (isConstant(curr->getOperand(0))) {
      EdgeValue lcnst(curr->getOperand(0));
      if (isConstant(curr->getOperand(1)) && isZeroValue(currNode)) {
        // Both const
        EdgeValue rcnst(curr->getOperand(1));
        auto ret = // join({lcnst}, {rcnst});
            performBinOp(binOp->getOpcode(), {lcnst}, {rcnst}, maxSetSize);
        return std::make_shared<GenConstant>(ret, maxSetSize);
      } else {
        // only lhs const
        return std::make_shared<BinaryEdgeFunction>(
            binOp->getOpcode(), v_t({lcnst}), true, maxSetSize);
      }
    } else if (!isConstant(curr->getOperand(1))) {
      // none const
      return std::make_shared<GenConstant>(bottomElement(), maxSetSize);
    } else {

      // only rhs const
      EdgeValue rcnst(curr->getOperand(1));
      return std::make_shared<BinaryEdgeFunction>(
          binOp->getOpcode(), v_t({rcnst}), false, maxSetSize);
    }
  } else if (auto cast = llvm::dyn_cast<llvm::CastInst>(curr);
             cast && curr == succNode) {
    if (cast->getDestTy()->isIntegerTy()) {
      auto destTy = llvm::cast<llvm::IntegerType>(cast->getDestTy());

      return std::make_shared<TypecastEdgeFunction>(
          destTy->getBitWidth(), EdgeValue::Integer, maxSetSize);
    } else if (cast->getDestTy()->isFloatingPointTy()) {
      auto bits = cast->getDestTy()->isFloatTy() ? 32 : 64;

      return std::make_shared<TypecastEdgeFunction>(
          bits, EdgeValue::FloatingPoint, maxSetSize);
    }
  }
  // std::cout << "FallThrough: identity edge fn" << std::endl;
  return IdentityEdgeFunction::getInstance(maxSetSize);
  // return std::make_shared<DebugIdentityEdgeFunction>(curr, succ,
  // maxSetSize);
}

std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::getCallEdgeFunction(
    IDELinearConstantPropagation::n_t callStmt,
    IDELinearConstantPropagation::d_t srcNode,
    IDELinearConstantPropagation::m_t destinationMethod,
    IDELinearConstantPropagation::d_t destNode) {
  // assert(destNode && "Invalid dest node");
  // assert(srcNode && "Invalid src node");
  if (!destNode) {
    return IdentityEdgeFunction::getInstance(maxSetSize);
  }
  /*if (isZeroValue(srcNode) &&
      destinationMethod->getName().contains("EVP_KDF_ctrl")) {
    std::cerr << "######################################################"
              << std::endl
              << destNode << std::endl;
  }*/
  // return edge-identity
  // return IdentityEdgeFunction::getInstance(maxSetSize);
  // return std::make_shared<MapFactsToCalleeEdgeFunction>(
  //    llvm::ImmutableCallSite(callStmt), srcNode, destinationMethod, destNode,
  //    maxSetSize);
  llvm::ImmutableCallSite cs(callStmt);
  auto len =
      std::min<size_t>(cs.getNumArgOperands(), destinationMethod->arg_size());

  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(srcNode)) {
    for (size_t i = 0; i < len; ++i) {
      auto formalArg = getNthFunctionArgument(destinationMethod, i);
      if (destNode == formalArg) {
        auto actualArg = cs.getArgOperand(i);
        // if (isConstant(actualArg))  // -> always const, since srcNode is zero
        return std::make_shared<GenConstant>(
            v_t({EdgeValue(cs.getArgOperand(i))}), maxSetSize);
        // else
        //  return IdentityEdgeFunction::getInstance(maxSetSize);
      }
    }
  }
  return IdentityEdgeFunction::getInstance(maxSetSize);
}

std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::getReturnEdgeFunction(
    IDELinearConstantPropagation::n_t callSite,
    IDELinearConstantPropagation::m_t calleeMethod,
    IDELinearConstantPropagation::n_t exitStmt,
    IDELinearConstantPropagation::d_t exitNode,
    IDELinearConstantPropagation::n_t reSite,
    IDELinearConstantPropagation::d_t retNode) {
  if (isZeroValue(exitNode)) {
    if (auto retStmt = llvm::dyn_cast<llvm::ReturnInst>(exitStmt)) {
      if (retStmt->getReturnValue() && isConstant(retStmt->getReturnValue())) {
        // std::cout << "Constant return value: "
        //          << psr::llvmIRToShortString(exitStmt) << std::endl;
        return std::make_shared<GenConstant>(
            v_t({EdgeValue(retStmt->getReturnValue())}), maxSetSize);
      }
    }
  }
  // std::cout << "Return identity: " << psr::llvmIRToShortString(exitStmt)
  //          << std::endl;
  // return edge-identity
  return IdentityEdgeFunction::getInstance(maxSetSize);
}
std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::getCallToRetEdgeFunction(
    IDELinearConstantPropagation::n_t callSite,
    IDELinearConstantPropagation::d_t callNode,
    IDELinearConstantPropagation::n_t retSite,
    IDELinearConstantPropagation::d_t retSiteNode,
    std::set<IDELinearConstantPropagation::m_t> callees) {
  // return edge-identity
  return IdentityEdgeFunction::getInstance(maxSetSize);
}

std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::getSummaryEdgeFunction(
    IDELinearConstantPropagation::n_t callStmt,
    IDELinearConstantPropagation::d_t callNode,
    IDELinearConstantPropagation::n_t retSite,
    IDELinearConstantPropagation::d_t retSiteNode) {
  // return edge-identity
  return IdentityEdgeFunction::getInstance(maxSetSize);
}

IDELinearConstantPropagation::v_t IDELinearConstantPropagation::topElement() {
  return v_t({});
}

IDELinearConstantPropagation::v_t
IDELinearConstantPropagation::bottomElement() {
  return v_t({EdgeValue::top});
}
IDELinearConstantPropagation::v_t
IDELinearConstantPropagation::join(IDELinearConstantPropagation::v_t lhs,
                                   IDELinearConstantPropagation::v_t rhs) {
  // sets are passed by value
  return ::join(lhs, rhs, maxSetSize);
}

std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
IDELinearConstantPropagation::allTopFunction() {
  static std::shared_ptr<EdgeFunction<IDELinearConstantPropagation::v_t>>
      alltopFn = std::make_shared<AllTop<v_t>>(topElement());
  return alltopFn;
}

void IDELinearConstantPropagation::printNode(
    std::ostream &os, IDELinearConstantPropagation::n_t n) const {
  os << llvmIRToString(n);
}

void IDELinearConstantPropagation::printDataFlowFact(
    std::ostream &os, IDELinearConstantPropagation::d_t d) const {
  assert(d && "Invalid dataflow fact");
  os << llvmIRToString(d);
}

void IDELinearConstantPropagation::printFunction(
    std::ostream &os, IDELinearConstantPropagation::m_t m) const {
  os << m->getName().str();
}

void IDELinearConstantPropagation::printEdgeFact(
    std::ostream &os, IDELinearConstantPropagation::v_t v) const {
  os << v;
}

/*void IDELinearConstantPropagation::printIDEReport(
    std::ostream &os,
    psr::SolverResults<IDELinearConstantPropagation::n_t,
                       IDELinearConstantPropagation::d_t,
                       IDELinearConstantPropagation::v_t> &SR) {

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

void IDELinearConstantPropagation::emitTextReport(
    const SolverResults<IDELinearConstantPropagation::n_t,
                        IDELinearConstantPropagation::d_t,
                        IDELinearConstantPropagation::v_t> &SR,
    std::ostream &os) {

  os << "\n====================== IDE-Linear-Constant-Analysis Report "
        "======================\n";
  if (!IRDB->debugInfoAvailable()) {
    // Emit only IR code, function name and module info
    os << "\nWARNING: No Debug Info available - emiting results without "
          "source code mapping!\n";
    for (auto f : ICF->getAllFunctions()) {
      std::string fName = getFunctionNameFromIR(f);
      os << "\nFunction: " << fName << "\n----------"
         << std::string(fName.size(), '-') << '\n';
      for (auto stmt : ICF->getAllInstructionsOf(f)) {
        auto results = SR.resultsAt(stmt, true);
        stripBottomResults(results);
        if (!results.empty()) {
          os << "At IR statement: " << NtoString(stmt) << '\n';
          for (auto res : results) {
            if (res.second != bottomElement()) {
              os << "   Fact: " << DtoString(res.first)
                 << "\n  Value: " << VtoString(res.second) << '\n';
            }
          }
          os << '\n';
        }
      }
      os << '\n';
    }
  } else {
    auto lcaResults = getLCAResults(SR);
    for (auto entry : lcaResults) {
      os << "\nFunction: " << entry.first
         << "\n==========" << std::string(entry.first.size(), '=') << '\n';
      for (auto fResult : entry.second) {
        fResult.second.print(os);
        os << "--------------------------------------\n\n";
      }
      os << '\n';
    }
  }
}

void IDELinearConstantPropagation::stripBottomResults(
    std::unordered_map<IDELinearConstantPropagation::d_t,
                       IDELinearConstantPropagation::v_t> &res) {
  for (auto it = res.begin(); it != res.end();) {
    if (it->second == bottomElement()) {
      it = res.erase(it);
    } else {
      ++it;
    }
  }
}
IDELinearConstantPropagation::lca_results_t
IDELinearConstantPropagation::getLCAResults(
    SolverResults<IDELinearConstantPropagation::n_t,
                  IDELinearConstantPropagation::d_t,
                  IDELinearConstantPropagation::v_t>
        SR) {
  std::map<std::string, std::map<unsigned, LCAResult>> aggResults;
  std::cout << "\n==== Computing LCA Results ====\n";
  for (auto f : ICF->getAllFunctions()) {
    std::string fName = getFunctionNameFromIR(f);
    std::cout << "\n-- Function: " << fName << " --\n";
    std::map<unsigned, LCAResult> fResults;
    std::set<std::string> allocatedVars;
    for (auto stmt : ICF->getAllInstructionsOf(f)) {
      unsigned lnr = getLineFromIR(stmt);
      std::cout << "\nIR : " << NtoString(stmt) << "\nLNR: " << lnr << '\n';
      // We skip statements with no source code mapping
      if (lnr == 0) {
        std::cout << "Skipping this stmt!\n";
        continue;
      }
      LCAResult *lcaRes = &fResults[lnr];
      // Check if it is a new result
      if (lcaRes->src_code.empty()) {
        std::string sourceCode = getSrcCodeFromIR(stmt);
        // Skip results for line containing only closed braces which is the
        // case for functions with void return value
        if (sourceCode == "}") {
          fResults.erase(lnr);
          continue;
        }
        lcaRes->src_code = sourceCode;
        lcaRes->line_nr = lnr;
      }
      lcaRes->ir_trace.push_back(stmt);
      if (stmt->isTerminator() && !ICF->isExitStmt(stmt)) {
        std::cout << "Delete result since stmt is Terminator or Exit!\n";
        fResults.erase(lnr);
      } else {
        // check results of succ(stmt)
        std::unordered_map<d_t, v_t> results;
        if (ICF->isExitStmt(stmt)) {
          results = SR.resultsAt(stmt, true);
        } else {
          // It's not a terminator inst, hence it has only a single successor
          auto succ = ICF->getSuccsOf(stmt)[0];
          std::cout << "Succ stmt: " << NtoString(succ) << '\n';
          results = SR.resultsAt(succ, true);
        }
        // stripBottomResults(results);
        std::set<std::string> validVarsAtStmt;
        for (auto res : results) {
          auto varName = getVarNameFromIR(res.first);
          std::cout << "  D: " << DtoString(res.first)
                    << " | V: " << VtoString(res.second)
                    << " | Var: " << varName << '\n';
          if (!varName.empty()) {
            // Only store/overwrite values of variables from allocas or
            // globals unless there is no value stored for a variable
            if (llvm::isa<llvm::AllocaInst>(res.first) ||
                llvm::isa<llvm::GlobalVariable>(res.first)) {
              // lcaRes->variableToValue.find(varName) ==
              // lcaRes->variableToValue.end()) {
              validVarsAtStmt.insert(varName);
              allocatedVars.insert(varName);
              lcaRes->variableToValue[varName] = res.second;
            } else if (allocatedVars.find(varName) == allocatedVars.end()) {
              validVarsAtStmt.insert(varName);
              lcaRes->variableToValue[varName] = res.second;
            }
          }
        }
        // remove no longer valid variables at current IR stmt
        for (auto it = lcaRes->variableToValue.begin();
             it != lcaRes->variableToValue.end();) {
          if (validVarsAtStmt.find(it->first) == validVarsAtStmt.end()) {
            std::cout << "Erase var: " << it->first << '\n';
            it = lcaRes->variableToValue.erase(it);
          } else {
            ++it;
          }
        }
      }
    }
    // delete entries with no result
    for (auto it = fResults.begin(); it != fResults.end();) {
      if (it->second.variableToValue.empty()) {
        it = fResults.erase(it);
      } else {
        ++it;
      }
    }
    aggResults[fName] = fResults;
  }
  return aggResults;
}

void IDELinearConstantPropagation::LCAResult::print(std::ostream &os) {
  os << "Line " << line_nr << ": " << src_code << '\n';
  os << "Var(s): ";
  for (auto it = variableToValue.begin(); it != variableToValue.end(); ++it) {
    if (it != variableToValue.begin()) {
      os << ", ";
    }
    os << it->first << " = " << it->second;
  }
  os << "\nCorresponding IR Instructions:\n";
  for (auto ir : ir_trace) {
    os << "  " << llvmIRToString(ir) << '\n';
  }
}
bool IDELinearConstantPropagation::isEntryPoint(const std::string &name) const {
  // For now, the only entrypoint is main
  return name == "main";
}
} // namespace psr