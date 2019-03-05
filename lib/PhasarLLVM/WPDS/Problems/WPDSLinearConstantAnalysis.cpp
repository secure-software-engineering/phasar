// /******************************************************************************
//  * Copyright (c) 2017 Philipp Schubert.
//  * All rights reserved. This program and the accompanying materials are made
//  * available under the terms of LICENSE.txt.
//  *
//  * Contributors:
//  *     Philipp Schubert and others
//  *****************************************************************************/

// #include <limits>
// #include <utility>

// #include <llvm/IR/Constants.h>
// #include <llvm/IR/Function.h>
// #include <llvm/IR/Instruction.h>
// #include <llvm/IR/Instructions.h>
// #include <llvm/IR/LLVMContext.h>
// #include <llvm/IR/Type.h>
// #include <llvm/IR/Value.h>

// #include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
// #include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>
// #include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenIf.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
// #include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
// #include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
// #include <phasar/PhasarLLVM/WPDS/Problems/WPDSLinearConstantAnalysis.h>
// #include <phasar/Utils/LLVMIRToSrc.h>
// #include <phasar/Utils/LLVMShorthands.h>
// #include <phasar/Utils/Logger.h>

// using namespace std;
// using namespace psr;

// namespace psr {
// // Initialize debug counter for edge functions
// unsigned WPDSLinearConstantAnalysis::CurrGenConstant_Id = 0;
// unsigned WPDSLinearConstantAnalysis::CurrLCAID_Id = 0;
// unsigned WPDSLinearConstantAnalysis::CurrBinary_Id = 0;

// const WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::TOP =
//     numeric_limits<WPDSLinearConstantAnalysis::v_t>::min();

// const WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::BOTTOM =
//     numeric_limits<WPDSLinearConstantAnalysis::v_t>::max();

// WPDSLinearConstantAnalysis::WPDSLinearConstantAnalysis(
//     LLVMBasedICFG &I, WPDSType WPDS, SearchDirection Direction,
//     std::vector<std::string> EntryPoints, std::vector<n_t> Stack,
//     bool Witnesses)
//     : WPDSProblem(I, WPDS, Direction, Stack, Witnesses),
//       EntryPoints(EntryPoints) {
//   zerovalue = LLVMZeroValue::getInstance();
// }

// WPDSLinearConstantAnalysis::~WPDSLinearConstantAnalysis() {
//   CurrGenConstant_Id = 0;
//   CurrLCAID_Id = 0;
//   CurrBinary_Id = 0;
// }

// // Start formulating our analysis by specifying the parts required for IFDS

// shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::getNormalFlowFunction(
//     WPDSLinearConstantAnalysis::n_t curr,
//     WPDSLinearConstantAnalysis::n_t succ) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getNormalFlowFunction()");
//   // Check store instructions. Store instructions override previous value
//   // of their pointer operand, i.e. kills previous fact (= pointer operand).
//   if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
//     WPDSLinearConstantAnalysis::d_t PointerOp = Store->getPointerOperand();
//     WPDSLinearConstantAnalysis::d_t ValueOp = Store->getValueOperand();
//     // Case I: Storing a constant integer.
//     if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
//       struct LCAFF : FlowFunction<WPDSLinearConstantAnalysis::d_t> {
//         WPDSLinearConstantAnalysis::d_t PointerOp, ZeroValue;
//         LCAFF(WPDSLinearConstantAnalysis::d_t PointerOperand,
//               WPDSLinearConstantAnalysis::d_t ZeroValue)
//             : PointerOp(PointerOperand), ZeroValue(ZeroValue) {}
//         set<WPDSLinearConstantAnalysis::d_t>
//         computeTargets(WPDSLinearConstantAnalysis::d_t source) override {
//           if (source == PointerOp) {
//             return {};
//           } else if (source == ZeroValue) {
//             return {source, PointerOp};
//           } else {
//             return {source};
//           }
//         }
//       };
//       return make_shared<LCAFF>(PointerOp, zeroValue());
//     }
//     // Case II: Storing an integer typed value.
//     if (ValueOp->getType()->isIntegerTy()) {
//       struct LCAFF : FlowFunction<WPDSLinearConstantAnalysis::d_t> {
//         WPDSLinearConstantAnalysis::d_t PointerOp, ValueOp;
//         LCAFF(WPDSLinearConstantAnalysis::d_t PointerOperand,
//               WPDSLinearConstantAnalysis::d_t ValueOperand)
//             : PointerOp(PointerOperand), ValueOp(ValueOperand) {}
//         set<WPDSLinearConstantAnalysis::d_t>
//         computeTargets(WPDSLinearConstantAnalysis::d_t source) override {
//           if (source == PointerOp) {
//             return {};
//           } else if (source == ValueOp) {
//             return {source, PointerOp};
//           } else {
//             return {source};
//           }
//         }
//       };
//       return make_shared<LCAFF>(PointerOp, ValueOp);
//     }
//   }
//   // check load instructions
//   if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
//     // only consider i32 load
//     if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
//       return make_shared<GenIf<WPDSLinearConstantAnalysis::d_t>>(
//           Load, zeroValue(), [Load](WPDSLinearConstantAnalysis::d_t source) {
//             return source == Load->getPointerOperand();
//           });
//     }
//   }
//   // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
//   if (llvm::isa<llvm::BinaryOperator>(curr)) {
//     auto lop = curr->getOperand(0);
//     auto rop = curr->getOperand(1);
//     return make_shared<GenIf<WPDSLinearConstantAnalysis::d_t>>(
//         curr, zeroValue(),
//         [this, lop, rop](WPDSLinearConstantAnalysis::d_t source) {
//           return (source != zerovalue &&
//                   ((lop == source && llvm::isa<llvm::ConstantInt>(rop)) ||
//                    (rop == source && llvm::isa<llvm::ConstantInt>(lop)))) ||
//                  (source == zerovalue && llvm::isa<llvm::ConstantInt>(lop) &&
//                   llvm::isa<llvm::ConstantInt>(rop));
//         });
//   }
//   return Identity<WPDSLinearConstantAnalysis::d_t>::getInstance();
// }

// shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::getCallFlowFunction(
//     WPDSLinearConstantAnalysis::n_t callStmt,
//     WPDSLinearConstantAnalysis::m_t destMthd) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                     << "WPDSLinearConstantAnalysis::getCallFlowFunction()";)
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//   // Map the actual parameters into the formal parameters
//   if (llvm::isa<llvm::CallInst>(callStmt) ||
//       llvm::isa<llvm::InvokeInst>(callStmt)) {
//     struct LCAFF : FlowFunction<const llvm::Value *> {
//       vector<const llvm::Value *> actuals;
//       vector<const llvm::Value *> formals;
//       LCAFF(llvm::ImmutableCallSite callSite,
//             WPDSLinearConstantAnalysis::m_t destMthd) {
//         // Set up the actual parameters
//         for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
//           actuals.push_back(callSite.getArgOperand(idx));
//         }
//         // Set up the formal parameters
//         for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
//           formals.push_back(getNthFunctionArgument(destMthd, idx));
//         }
//       }
//       set<WPDSLinearConstantAnalysis::d_t>
//       computeTargets(WPDSLinearConstantAnalysis::d_t source) override {
//         set<WPDSLinearConstantAnalysis::d_t> res;
//         for (unsigned idx = 0; idx < actuals.size(); ++idx) {
//           // Ordinary case: Just perform mapping
//           if (source == actuals[idx]) {
//             res.insert(formals[idx]); // corresponding formal
//           }
//           // Special case: Check if function is called with integer literals as
//           // parameter
//           if (isLLVMZeroValue(source) &&
//               llvm::isa<llvm::ConstantInt>(actuals[idx])) {
//             res.insert(formals[idx]); // corresponding formal
//           }
//         }
//         return res;
//       }
//     };
//     return make_shared<LCAFF>(llvm::ImmutableCallSite(callStmt), destMthd);
//   }
//   // Pass everything else as identity
//   return Identity<WPDSLinearConstantAnalysis::d_t>::getInstance();
// }

// shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::getRetFlowFunction(
//     WPDSLinearConstantAnalysis::n_t callSite,
//     WPDSLinearConstantAnalysis::m_t calleeMthd,
//     WPDSLinearConstantAnalysis::n_t exitStmt,
//     WPDSLinearConstantAnalysis::n_t retSite) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getRetFlowFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//   // Handle the case: %x = call i32 ...
//   if (callSite->getType()->isIntegerTy()) {
//     auto Return = llvm::dyn_cast<llvm::ReturnInst>(exitStmt);
//     auto ReturnValue = Return->getReturnValue();
//     struct LCAFF : FlowFunction<WPDSLinearConstantAnalysis::d_t> {
//       WPDSLinearConstantAnalysis::n_t callSite;
//       WPDSLinearConstantAnalysis::d_t ReturnValue;
//       LCAFF(WPDSLinearConstantAnalysis::n_t cs,
//             WPDSLinearConstantAnalysis::d_t retVal)
//           : callSite(cs), ReturnValue(retVal) {}
//       set<WPDSLinearConstantAnalysis::d_t>
//       computeTargets(WPDSLinearConstantAnalysis::d_t source) override {
//         set<WPDSLinearConstantAnalysis::d_t> res;
//         // Collect return value fact
//         if (source == ReturnValue) {
//           res.insert(callSite);
//         }
//         // Return value is integer literal
//         if (isLLVMZeroValue(source) &&
//             llvm::isa<llvm::ConstantInt>(ReturnValue)) {
//           res.insert(callSite);
//         }
//         return res;
//       }
//     };
//     return make_shared<LCAFF>(callSite, ReturnValue);
//   }
//   // All other facts are killed at this point
//   return KillAll<WPDSLinearConstantAnalysis::d_t>::getInstance();
// }

// shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::getCallToRetFlowFunction(
//     WPDSLinearConstantAnalysis::n_t callSite,
//     WPDSLinearConstantAnalysis::n_t retSite, set<m_t> callees) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getCallToRetFlowFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//   return Identity<WPDSLinearConstantAnalysis::d_t>::getInstance();
// }

// shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::getSummaryFlowFunction(
//     WPDSLinearConstantAnalysis::n_t callStmt,
//     WPDSLinearConstantAnalysis::m_t destMthd) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getSummaryFlowFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//   return nullptr;
// }

// // map<WPDSLinearConstantAnalysis::n_t, set<WPDSLinearConstantAnalysis::d_t>>
// // WPDSLinearConstantAnalysis::initialSeeds() {
// //   // Check commandline arguments, e.g. argc, and generate all integer
// //   // typed arguments.
// //   map<WPDSLinearConstantAnalysis::n_t, set<WPDSLinearConstantAnalysis::d_t>>
// //       SeedMap;
// //   for (auto &EntryPoint : EntryPoints) {
// //     if (EntryPoint == "main") {
// //       set<WPDSLinearConstantAnalysis::d_t> CmdArgs;
// //       for (auto &Arg : icfg.getMethod(EntryPoint)->args()) {
// //         if (Arg.getType()->isIntegerTy()) {
// //           CmdArgs.insert(&Arg);
// //         }
// //       }
// //       CmdArgs.insert(zeroValue());
// //       SeedMap.insert(
// //           make_pair(&icfg.getMethod(EntryPoint)->front().front(), CmdArgs));
// //     } else {
// //       SeedMap.insert(
// //           make_pair(&icfg.getMethod(EntryPoint)->front().front(),
// //                     set<WPDSLinearConstantAnalysis::d_t>({zeroValue()})));
// //     }
// //   }
// //   return SeedMap;
// // }

// // WPDSLinearConstantAnalysis::d_t WPDSLinearConstantAnalysis::createZeroValue()
// // {
// //   // create a special value to represent the zero value!
// //   return LLVMZeroValue::getInstance();
// // }

// bool WPDSLinearConstantAnalysis::isZeroValue(
//     WPDSLinearConstantAnalysis::d_t d) const {
//   return isLLVMZeroValue(d);
// }

// // In addition provide specifications for the IDE parts

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::getNormalEdgeFunction(
//     WPDSLinearConstantAnalysis::n_t curr,
//     WPDSLinearConstantAnalysis::d_t currNode,
//     WPDSLinearConstantAnalysis::n_t succ,
//     WPDSLinearConstantAnalysis::d_t succNode) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getNormalEdgeFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Curr Inst : "
//                 << WPDSLinearConstantAnalysis::NtoString(curr));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Curr Node :   "
//                 << WPDSLinearConstantAnalysis::DtoString(currNode));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Succ Inst : "
//                 << WPDSLinearConstantAnalysis::NtoString(succ));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Succ Node :   "
//                 << WPDSLinearConstantAnalysis::DtoString(succNode));
//   // All_Bottom for zero value
//   if (isZeroValue(currNode) && isZeroValue(succNode)) {
//     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Zero value.");
//     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//     return make_shared<AllBottom<WPDSLinearConstantAnalysis::v_t>>(
//         bottomElement());
//   }
//   // Check store instruction
//   if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
//     WPDSLinearConstantAnalysis::d_t pointerOperand = Store->getPointerOperand();
//     WPDSLinearConstantAnalysis::d_t valueOperand = Store->getValueOperand();
//     if (pointerOperand == succNode) {
//       // Case I: Storing a constant integer.
//       if (isZeroValue(currNode) && llvm::isa<llvm::ConstantInt>(valueOperand)) {
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                       << "Case: Storing constant integer.");
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//         auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand);
//         auto IntConst = CI->getSExtValue();
//         return make_shared<WPDSLinearConstantAnalysis::GenConstant>(IntConst);
//       }
//       // Case II: Storing an integer typed value.
//       if (currNode != succNode && valueOperand->getType()->isIntegerTy()) {
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                       << "Case: Storing an integer typed value.");
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//         return make_shared<WPDSLinearConstantAnalysis::LCAIdentity>();
//       }
//     }
//   }

//   // Check load instruction
//   if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
//     if (Load == succNode) {
//       LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                     << "Case: Loading an integer typed value.");
//       LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//       return make_shared<WPDSLinearConstantAnalysis::LCAIdentity>();
//     }
//   }
//   // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
//   if (curr == succNode && llvm::isa<llvm::BinaryOperator>(curr)) {
//     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Binary operation.");
//     LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//     unsigned OP = curr->getOpcode();
//     auto lop = curr->getOperand(0);
//     auto rop = curr->getOperand(1);
//     struct LCAEF : EdgeFunction<WPDSLinearConstantAnalysis::v_t>,
//                    enable_shared_from_this<LCAEF> {
//       const unsigned EdgeFunctionID, Op;
//       WPDSLinearConstantAnalysis::d_t lop, rop, currNode;
//       LCAEF(const unsigned Op, WPDSLinearConstantAnalysis::d_t lop,
//             WPDSLinearConstantAnalysis::d_t rop,
//             WPDSLinearConstantAnalysis::d_t currNode)
//           : EdgeFunctionID(++WPDSLinearConstantAnalysis::CurrBinary_Id), Op(Op),
//             lop(lop), rop(rop), currNode(currNode) {}

//       WPDSLinearConstantAnalysis::v_t
//       computeTarget(WPDSLinearConstantAnalysis::v_t source) override {
//         auto &lg = lg::get();
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                       << "Left Op   : " << llvmIRToString(lop));
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                       << "Right Op  : " << llvmIRToString(rop));
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                       << "Curr Node : " << llvmIRToString(currNode));
//         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//         if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
//           auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
//           return WPDSLinearConstantAnalysis::executeBinOperation(
//               Op, source, ric->getSExtValue());
//         } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
//           auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
//           return WPDSLinearConstantAnalysis::executeBinOperation(
//               Op, lic->getSExtValue(), source);
//         } else if (isLLVMZeroValue(currNode) &&
//                    llvm::isa<llvm::ConstantInt>(lop) &&
//                    llvm::isa<llvm::ConstantInt>(rop)) {
//           auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
//           auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
//           return WPDSLinearConstantAnalysis::executeBinOperation(
//               Op, lic->getSExtValue(), ric->getSExtValue());
//         }
//         throw runtime_error(
//             "Only linear constant propagation can be specified!");
//       }

//       shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
//       composeWith(shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
//                       secondFunction) override {
//         if (auto *EI =
//                 dynamic_cast<EdgeIdentity<WPDSLinearConstantAnalysis::v_t> *>(
//                     secondFunction.get())) {
//           return this->shared_from_this();
//         }
//         if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
//           return this->shared_from_this();
//         }
//         return make_shared<WPDSLinearConstantAnalysis::LCAEdgeFunctionComposer>(
//             this->shared_from_this(), secondFunction);
//       }

//       shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
//       joinWith(shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
//                    otherFunction) override {
//         if (otherFunction.get() == this ||
//             otherFunction->equal_to(this->shared_from_this())) {
//           return this->shared_from_this();
//         }
//         if (auto *AT = dynamic_cast<AllTop<WPDSLinearConstantAnalysis::v_t> *>(
//                 otherFunction.get())) {
//           return this->shared_from_this();
//         }
//         return make_shared<AllBottom<WPDSLinearConstantAnalysis::v_t>>(
//             WPDSLinearConstantAnalysis::BOTTOM);
//       }

//       bool equal_to(shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
//                         other) const override {
//         // could be more precise - compare op,lop and rop?
//         return this == other.get();
//       }

//       void print(ostream &OS, bool isForDebug = false) const override {
//         OS << "Binary_" << EdgeFunctionID;
//       }
//     };
//     return make_shared<LCAEF>(OP, lop, rop, currNode);
//   }
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Edge identity.");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
//   return EdgeIdentity<WPDSLinearConstantAnalysis::v_t>::getInstance();
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::getCallEdgeFunction(
//     WPDSLinearConstantAnalysis::n_t callStmt,
//     WPDSLinearConstantAnalysis::d_t srcNode,
//     WPDSLinearConstantAnalysis::m_t destiantionMethod,
//     WPDSLinearConstantAnalysis::d_t destNode) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getCallEdgeFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Call Stmt   : "
//                 << WPDSLinearConstantAnalysis::NtoString(callStmt));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Src Node    :   "
//                 << WPDSLinearConstantAnalysis::DtoString(srcNode));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(M) Dest Method : "
//                 << WPDSLinearConstantAnalysis::MtoString(destiantionMethod));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Dest Node   :   "
//                 << WPDSLinearConstantAnalysis::DtoString(destNode));
//   // Case: Passing constant integer as parameter
//   if (isZeroValue(srcNode) && !isZeroValue(destNode)) {
//     if (auto A = llvm::dyn_cast<llvm::Argument>(destNode)) {
//       llvm::ImmutableCallSite CS(callStmt);
//       auto actual = CS.getArgOperand(getFunctionArgumentNr(A));
//       if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(actual)) {
//         auto IntConst = CI->getSExtValue();
//         return make_shared<WPDSLinearConstantAnalysis::GenConstant>(IntConst);
//       }
//     }
//   }
//   return EdgeIdentity<WPDSLinearConstantAnalysis::v_t>::getInstance();
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::getReturnEdgeFunction(
//     WPDSLinearConstantAnalysis::n_t callSite,
//     WPDSLinearConstantAnalysis::m_t calleeMethod,
//     WPDSLinearConstantAnalysis::n_t exitStmt,
//     WPDSLinearConstantAnalysis::d_t exitNode,
//     WPDSLinearConstantAnalysis::n_t reSite,
//     WPDSLinearConstantAnalysis::d_t retNode) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getReturnEdgeFunction()");
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Call Site : "
//                 << WPDSLinearConstantAnalysis::NtoString(callSite));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(M) Callee    : "
//                 << WPDSLinearConstantAnalysis::MtoString(calleeMethod));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Exit Stmt : "
//                 << WPDSLinearConstantAnalysis::NtoString(exitStmt));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Exit Node :   "
//                 << WPDSLinearConstantAnalysis::DtoString(exitNode));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(N) Ret Site  : "
//                 << WPDSLinearConstantAnalysis::NtoString(reSite));
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "(D) Ret Node  :   "
//                 << WPDSLinearConstantAnalysis::DtoString(retNode));
//   // Case: Returning constant integer
//   if (isZeroValue(exitNode) && !isZeroValue(retNode)) {
//     auto Return = llvm::dyn_cast<llvm::ReturnInst>(exitStmt);
//     auto ReturnValue = Return->getReturnValue();
//     if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(ReturnValue)) {
//       auto IntConst = CI->getSExtValue();
//       return make_shared<WPDSLinearConstantAnalysis::GenConstant>(IntConst);
//     }
//   }
//   return EdgeIdentity<WPDSLinearConstantAnalysis::v_t>::getInstance();
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::getCallToRetEdgeFunction(
//     WPDSLinearConstantAnalysis::n_t callSite,
//     WPDSLinearConstantAnalysis::d_t callNode,
//     WPDSLinearConstantAnalysis::n_t retSite,
//     WPDSLinearConstantAnalysis::d_t retSiteNode,
//     set<WPDSLinearConstantAnalysis::m_t> callees) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getCallToRetEdgeFunction()");
//   return EdgeIdentity<WPDSLinearConstantAnalysis::v_t>::getInstance();
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::getSummaryEdgeFunction(
//     WPDSLinearConstantAnalysis::n_t callStmt,
//     WPDSLinearConstantAnalysis::d_t callNode,
//     WPDSLinearConstantAnalysis::n_t retSite,
//     WPDSLinearConstantAnalysis::d_t retSiteNode) {
//   auto &lg = lg::get();
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
//                 << "WPDSLinearConstantAnalysis::getSummaryEdgeFunction()");
//   return EdgeIdentity<WPDSLinearConstantAnalysis::v_t>::getInstance();
// }

// WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::topElement() {
//   return TOP;
// }

// WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::bottomElement() {
//   return BOTTOM;
// }

// WPDSLinearConstantAnalysis::v_t
// WPDSLinearConstantAnalysis::join(WPDSLinearConstantAnalysis::v_t lhs,
//                                  WPDSLinearConstantAnalysis::v_t rhs) {
//   if (lhs == TOP && rhs != BOTTOM) {
//     return rhs;
//   } else if (rhs == TOP && lhs != BOTTOM) {
//     return lhs;
//   } else if (rhs == lhs) {
//     return rhs;
//   } else {
//     return BOTTOM;
//   }
// }

// WPDSLinearConstantAnalysis::d_t WPDSLinearConstantAnalysis::zeroValue() {
//   return zerovalue;
// }

// std::map<WPDSLinearConstantAnalysis::n_t,
//          std::set<WPDSLinearConstantAnalysis::d_t>>
// WPDSLinearConstantAnalysis::initialSeeds() {
//   // Check commandline arguments, e.g. argc, and generate all integer
//   // typed arguments.
//   map<WPDSLinearConstantAnalysis::n_t, set<WPDSLinearConstantAnalysis::d_t>>
//       SeedMap;
//   for (auto &EntryPoint : EntryPoints) {
//     if (EntryPoint == "main") {
//       set<WPDSLinearConstantAnalysis::d_t> CmdArgs;
//       for (auto &Arg : ICFG.getMethod(EntryPoint)->args()) {
//         if (Arg.getType()->isIntegerTy()) {
//           CmdArgs.insert(&Arg);
//         }
//       }
//       CmdArgs.insert(zeroValue());
//       SeedMap.insert(
//           make_pair(&ICFG.getMethod(EntryPoint)->front().front(), CmdArgs));
//     } else {
//       SeedMap.insert(
//           make_pair(&ICFG.getMethod(EntryPoint)->front().front(),
//                     set<WPDSLinearConstantAnalysis::d_t>({zeroValue()})));
//     }
//   }
//   return SeedMap;
// }

// std::shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::allTopFunction() {
//   return make_shared<AllTop<WPDSLinearConstantAnalysis::v_t>>(TOP);
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::LCAEdgeFunctionComposer::composeWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> secondFunction) {
//   if (auto *EI = dynamic_cast<EdgeIdentity<WPDSLinearConstantAnalysis::v_t> *>(
//           secondFunction.get())) {
//     return this->shared_from_this();
//   }
//   if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
//     return this->shared_from_this();
//   }
//   return F->composeWith(G->composeWith(secondFunction));
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::LCAEdgeFunctionComposer::joinWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> otherFunction) {
//   if (otherFunction.get() == this ||
//       otherFunction->equal_to(this->shared_from_this())) {
//     return this->shared_from_this();
//   }
//   if (auto *AT = dynamic_cast<AllTop<WPDSLinearConstantAnalysis::v_t> *>(
//           otherFunction.get())) {
//     return this->shared_from_this();
//   }
//   return make_shared<AllBottom<WPDSLinearConstantAnalysis::v_t>>(
//       WPDSLinearConstantAnalysis::BOTTOM);
// }

// WPDSLinearConstantAnalysis::GenConstant::GenConstant(
//     WPDSLinearConstantAnalysis::v_t IntConst)
//     : GenConstant_Id(++WPDSLinearConstantAnalysis::CurrGenConstant_Id),
//       IntConst(IntConst) {}

// WPDSLinearConstantAnalysis::v_t
// WPDSLinearConstantAnalysis::GenConstant::computeTarget(
//     WPDSLinearConstantAnalysis::v_t source) {
//   return IntConst;
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::GenConstant::composeWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> secondFunction) {
//   if (auto *EI = dynamic_cast<EdgeIdentity<WPDSLinearConstantAnalysis::v_t> *>(
//           secondFunction.get())) {
//     return this->shared_from_this();
//   }
//   if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
//     return this->shared_from_this();
//   }
//   return make_shared<WPDSLinearConstantAnalysis::LCAEdgeFunctionComposer>(
//       this->shared_from_this(), secondFunction);
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::GenConstant::joinWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> otherFunction) {
//   if (otherFunction.get() == this ||
//       otherFunction->equal_to(this->shared_from_this())) {
//     return this->shared_from_this();
//   }
//   return make_shared<AllBottom<WPDSLinearConstantAnalysis::v_t>>(
//       WPDSLinearConstantAnalysis::BOTTOM);
// }

// bool WPDSLinearConstantAnalysis::GenConstant::equal_to(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> other) const {
//   if (auto *StC = dynamic_cast<WPDSLinearConstantAnalysis::GenConstant *>(
//           other.get())) {
//     return (StC->IntConst == this->IntConst);
//   }
//   return this == other.get();
// }

// void WPDSLinearConstantAnalysis::GenConstant::print(ostream &OS,
//                                                     bool isForDebug) const {
//   OS << "GenConstant_" << GenConstant_Id;
// }

// WPDSLinearConstantAnalysis::LCAIdentity::LCAIdentity()
//     : LCAID_Id(++WPDSLinearConstantAnalysis::CurrLCAID_Id) {}

// WPDSLinearConstantAnalysis::v_t
// WPDSLinearConstantAnalysis::LCAIdentity::computeTarget(
//     WPDSLinearConstantAnalysis::v_t source) {
//   return source;
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::LCAIdentity::composeWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> secondFunction) {
//   return secondFunction;
// }

// shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
// WPDSLinearConstantAnalysis::LCAIdentity::joinWith(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> otherFunction) {
//   if (otherFunction.get() == this ||
//       otherFunction->equal_to(this->shared_from_this())) {
//     return this->shared_from_this();
//   }
//   if (auto *AT = dynamic_cast<AllTop<WPDSLinearConstantAnalysis::v_t> *>(
//           otherFunction.get())) {
//     return this->shared_from_this();
//   }
//   return make_shared<AllBottom<WPDSLinearConstantAnalysis::v_t>>(
//       WPDSLinearConstantAnalysis::BOTTOM);
// }

// bool WPDSLinearConstantAnalysis::LCAIdentity::equal_to(
//     shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>> other) const {
//   return this == other.get();
// }

// void WPDSLinearConstantAnalysis::LCAIdentity::print(ostream &OS,
//                                                     bool isForDebug) const {
//   OS << "LCAIdentity_" << LCAID_Id;
// }

// WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::executeBinOperation(
//     const unsigned op, WPDSLinearConstantAnalysis::v_t lop,
//     WPDSLinearConstantAnalysis::v_t rop) {
//   WPDSLinearConstantAnalysis::v_t res;
//   switch (op) {
//   case llvm::Instruction::Add:
//     res = lop + rop;
//     break;

//   case llvm::Instruction::Sub:
//     res = lop - rop;
//     break;

//   case llvm::Instruction::Mul:
//     res = lop * rop;
//     break;

//   case llvm::Instruction::UDiv:
//   case llvm::Instruction::SDiv:
//     res = lop / rop;
//     break;

//   case llvm::Instruction::URem:
//   case llvm::Instruction::SRem:
//     res = lop % rop;
//     break;

//   default:
//     throw runtime_error("Could not execute unknown operation '" +
//                         to_string(op) + "'!");
//   }
//   return res;
// }

// // void WPDSLinearConstantAnalysis::printNode(
// //     ostream &os, WPDSLinearConstantAnalysis::n_t n) const {
// //   os << llvmIRToString(n);
// // }

// // void WPDSLinearConstantAnalysis::printDataFlowFact(
// //     ostream &os, WPDSLinearConstantAnalysis::d_t d) const {
// //   os << llvmIRToString(d);
// // }

// // void WPDSLinearConstantAnalysis::printMethod(
// //     ostream &os, WPDSLinearConstantAnalysis::m_t m) const {
// //   os << m->getName().str();
// // }

// // void WPDSLinearConstantAnalysis::printValue(
// //     ostream &os, WPDSLinearConstantAnalysis::v_t v) const {
// //   os << ((v == BOTTOM) ? "Bottom" : to_string(v));
// // }

// // void WPDSLinearConstantAnalysis::printIDEReport(
// //     std::ostream &os, SolverResults<WPDSLinearConstantAnalysis::n_t,
// //                                     WPDSLinearConstantAnalysis::d_t,
// //                                     WPDSLinearConstantAnalysis::v_t> &SR) {
// //   os << "\n======= LCA RESULTS =======\n";
// //   for (auto f : icfg.getAllMethods()) {
// //     os << llvmFunctionToSrc(f) << '\n';
// //     for (auto exit : icfg.getExitPointsOf(f)) {
// //       auto results = SR.resultsAt(exit, true);
// //       if (results.empty()) {
// //         os << "\nNo results available!\n";
// //       } else {
// //         for (auto res : results) {
// //           if (!llvm::isa<llvm::LoadInst>(res.first)) {
// //             os << "\nValue: " << VtoString(res.second)
// //                << "\nIR  : " << DtoString(res.first) << '\n'
// //                << llvmValueToSrc(res.first, false) << "\n";
// //           }
// //         }
// //       }
// //     }
// //     os << "----------------\n";
// //   }
// // }

// void WPDSLinearConstantAnalysis::printNode(
//     ostream &os, WPDSLinearConstantAnalysis::n_t n) const {
//   os << llvmIRToString(n);
// }

// void WPDSLinearConstantAnalysis::printDataFlowFact(
//     ostream &os, WPDSLinearConstantAnalysis::d_t d) const {
//   os << llvmIRToString(d);
// }

// void WPDSLinearConstantAnalysis::printMethod(
//     ostream &os, WPDSLinearConstantAnalysis::m_t m) const {
//   os << m->getName().str();
// }

// void WPDSLinearConstantAnalysis::printValue(
//     ostream &os, WPDSLinearConstantAnalysis::v_t v) const {
//   os << ((v == BOTTOM) ? "Bottom" : to_string(v));
// }

// } // namespace psr
