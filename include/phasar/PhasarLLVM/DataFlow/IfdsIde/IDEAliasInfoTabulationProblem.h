#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDEAliasInfoTabulationProblem
    : public IDETabulationProblem<AnalysisDomainTy, Container> {
public:
  using ProblemAnalysisDomain = AnalysisDomainTy;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using db_t = typename AnalysisDomainTy::db_t;

  using ConfigurationTy = HasNoConfigurationType;

  using FlowFunctionType = FlowFunction<d_t, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using container_type = typename FlowFunctionType::container_type;

  explicit IDEAliasInfoTabulationProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasInfoRef PT,
      std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy, Container>(
            IRDB, std::move(EntryPoints), std::move(ZeroValue),
            NullAnalysisPrinter<AnalysisDomainTy>::getInstance()),
        PT(PT) {
    assert(IRDB != nullptr);
  }

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t /*Succ*/) override {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      return generateFlowIf(Load, [Load](d_t Source) {
        return Source == Load->getPointerOperand();
      });
    }
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      container_type Gen;
      auto AliasSet = PT.getAliasSet(Store->getPointerOperand(), Store);
      Gen.insert(AliasSet->begin(), AliasSet->end());

      return lambdaFlow(
          [Store, Gen{std::move(Gen)}](d_t Source) -> container_type {
            if (Store->getPointerOperand() == Source) {
              return {};
            }
            if (Store->getValueOperand() == Source) {
              return Gen;
            }

            return {Source};
          });
    }
    if (const auto *UnaryOp = llvm::dyn_cast<llvm::UnaryOperator>(Curr)) {
      return generateFlow(UnaryOp, UnaryOp->getOperand(0));
    }
    if (const auto *BinaryOp = llvm::dyn_cast<llvm::BinaryOperator>(Curr)) {
      return generateFlowIf(BinaryOp, [BinaryOp](d_t Source) {
        return Source == BinaryOp->getLeftOp() ||
               Source == BinaryOp->getRightOp();
      });
    }
    if (const auto *GetElementPtr =
            llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
      return generateFlow(GetElementPtr, GetElementPtr->getPointerOperand());
    }

    return this->identityFlow();
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallInst,
                                          f_t CalleeFun) override {
    return mapFactsToCallee(CallInst, CalleeFun, [](d_t Actual, d_t Source) {
      return Actual == Source && Actual->getType()->isPointerTy();
    });
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                         n_t ExitInst,
                                         n_t /*RetSite*/) override {
    container_type Gen;
    auto AliasSet = PT.getAliasSet(CallSite->getPointerOperand(), CallSite);
    Gen.insert(AliasSet->begin(), AliasSet->end());

    // TODO: Entweder Lambda Funktionen als variablen speichern und übergeben,
    // oder die Funktionen sind vllt einfach sowas wie generateFlow?
    // TODO: Tests schreiben für IDEAliasInfoTabulationProblem und
    // IDENoAliasInfoTabulationProblem default flow functions!
    // Hierzu vielleicht einfach eine cpp Datei, die Stumpf nur die default flow
    // functions nimmt und darüber tests schreiben.

    return lambdaFlow([CallSite, ExitInst,
                       Gen{std::move(Gen)}](d_t Source) -> container_type {
      auto PropArg = [](d_t Arg, d_t Source) {
        return Arg == Source && Arg->getType()->isPointerTy();
      };
      auto FactConstructor = [](int a, int b) { return a + b; };

      Container Res;

      if (ExitInst->getInt() && LLVMZeroValue::isLLVMZeroValue(Source)) {
        Res.insert(Source);
        return Res;
      }

      if (CallSite->getInt() && llvm::isa<llvm::Constant>(Source)) {
        // Pass global variables as is, if desired
        // Globals could also be actual arguments, then the formal
        // argument needs to be generated below. Need llvm::Constant here
        // to cover also ConstantExpr and ConstantAggregate
        Res.insert(Source);
      }

      const auto *CS = CallSite->getPointer();
      const auto *DestFun = ExitInst->getPointer()->getFunction();
      assert(CS->arg_size() >= DestFun->arg_size());
      assert(CS->arg_size() == DestFun->arg_size() || DestFun->isVarArg());

      llvm::CallBase::const_op_iterator ArgIt = CS->arg_begin();
      llvm::CallBase::const_op_iterator ArgEnd = CS->arg_end();
      llvm::Function::const_arg_iterator ParamIt = DestFun->arg_begin();
      llvm::Function::const_arg_iterator ParamEnd = DestFun->arg_end();

      for (; ParamIt != ParamEnd; ++ParamIt, ++ArgIt) {
        if (ArgIt->get()->getType()->isPointerTy()) {
          if (std::invoke(PropArg, &*ParamIt, Source)) {
            Res.insert(std::invoke(FactConstructor, ArgIt->get()));
          }
        }
      }

      if (ArgIt != ArgEnd) {
        // Over-approximate by trying to add the
        //   alloca [1 x %struct.__va_list_tag], align 16
        // to the results
        // find the allocated %struct.__va_list_tag and generate it

        for (const auto &I : llvm::instructions(DestFun)) {
          if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
            const auto *AllocTy = Alloc->getAllocatedType();
            if (AllocTy->isArrayTy() && AllocTy->getArrayNumElements() > 0 &&
                AllocTy->getArrayElementType()->isStructTy() &&
                AllocTy->getArrayElementType()->getStructName() ==
                    "struct.__va_list_tag") {
              if (std::invoke(PropArg, Alloc, Source)) {
                std::transform(ArgIt, ArgEnd, std::inserter(Res, Res.end()),
                               FactConstructor);
                break;
              }
            }
          }
        }
      }

      if (const auto *RetInst =
              llvm::dyn_cast<llvm::ReturnInst>(ExitInst->getPointer());
          RetInst && RetInst->getReturnValue()) {
        if (std::invoke(
                [](d_t RetVal, d_t Source) {
                  return RetVal == Source ||
                         (LLVMZeroValue::isLLVMZeroValue(Source) &&
                          llvm::isa<llvm::ConstantInt>(RetVal));
                },
                RetInst->getReturnValue(), Source)) {
          Res.insert(std::invoke(FactConstructor, CS));
        }
      }

      std::invoke(
          [CallSite](container_type &Res) {
            // Correctly handling return-POIs
            populateWithMayAliases(Res, CallSite);
          },
          Res);

      return Res;
    }

    );
  }
  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t /*RetSite*/,
                           llvm::ArrayRef<f_t> Callees) override {
    // TODO: alle pointer killen und alle globals
    // Bei declaration only function können wir nicht davon ausgehen, dass der
    // pointer gekillt wird außer bei Funktionen die der analyse bekannt sind.
    //

    // If any callee is a declaration, return identity
    if (llvm::any_of(Callees, llvm::isa<llvm::PointerType>)) {
      return this->identityFlow();
    }

    return mapFactsAlongsideCallSite(
        CallSite,
        [](d_t Arg, d_t Source) {
          return Arg == Source && Arg->getType()->isPointerTy();
        },
        false);
  }

private:
  LLVMAliasInfoRef PT;
};

} // namespace psr

#endif
