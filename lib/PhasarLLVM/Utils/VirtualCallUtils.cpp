#include "phasar/PhasarLLVM/Utils/VirtualCallUtils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

using namespace psr;

std::optional<unsigned> psr::getVFTIndex(const llvm::CallBase *CallSite) {
  // deal with a virtual member function
  // retrieve the vtable entry that is called
  const auto *Load =
      llvm::dyn_cast<llvm::LoadInst>(CallSite->getCalledOperand());
  if (Load == nullptr) {
    return std::nullopt;
  }
  const auto *GEP =
      llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand());
  if (GEP == nullptr) {
    return std::nullopt;
  }
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(1))) {
    return CI->getZExtValue();
  }
  return std::nullopt;
}

std::optional<std::pair<const llvm::Value *, uint64_t>>
psr::getVFTIndexAndVT(const llvm::CallBase *CallSite) {
  // deal with a virtual member function
  // retrieve the vtable entry that is called
  const auto *Load =
      llvm::dyn_cast<llvm::LoadInst>(CallSite->getCalledOperand());
  if (Load == nullptr) {
    return std::nullopt;
  }

  const auto *GEP =
      llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand());
  // Vtable GEPs index into a pointer array with a single index.
  // Multi-index GEPs (e.g. struct field access) are not vtable patterns.
  if (GEP == nullptr || GEP->getNumOperands() != 2) {
    return std::nullopt;
  }

  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(1))) {
    return {{GEP->getPointerOperand(), CI->getZExtValue()}};
  }

  return std::nullopt;
}

std::optional<std::tuple<const llvm::Value *, llvm::SmallVector<uint64_t, 3>,
                         llvm::Type *>>
psr::getConstGEPFieldAccess(const llvm::Value *PtrOperand) {
  const auto *GEP = llvm::dyn_cast<llvm::GEPOperator>(PtrOperand);
  if (!GEP || GEP->getNumOperands() < 3 || !GEP->hasAllConstantIndices()) {
    return std::nullopt;
  }
  llvm::SmallVector<uint64_t, 3> Indices;
  for (const llvm::Use &Idx : GEP->indices()) {
    Indices.push_back(llvm::cast<llvm::ConstantInt>(Idx.get())->getZExtValue());
  }
  return {{GEP->getPointerOperand(), std::move(Indices),
           GEP->getSourceElementType()}};
}

std::optional<std::tuple<const llvm::Value *, llvm::SmallVector<uint64_t, 3>,
                         llvm::Type *>>
psr::getStructVCallInfo(const llvm::CallBase *CallSite) {
  const auto *Load =
      llvm::dyn_cast<llvm::LoadInst>(CallSite->getCalledOperand());
  if (!Load) {
    return std::nullopt;
  }
  return getConstGEPFieldAccess(Load->getPointerOperand());
}

bool psr::isConsistentCall(const llvm::CallBase *CallSite,
                           const llvm::Function *DestFun) {
  if (CallSite->arg_size() < DestFun->arg_size()) {
    return false;
  }
  if (CallSite->arg_size() != DestFun->arg_size() && !DestFun->isVarArg()) {
    return false;
  }

  for (const auto &[Param, ArgOp] :
       llvm::zip_first(DestFun->args(), CallSite->args())) {

    const auto *ParamTy = Param.getType();
    const auto *ArgTy = ArgOp->getType();

    if (ParamTy == ArgTy) {
      // Trivial equality
      continue;
    }

    if (ParamTy->getTypeID() != ArgTy->getTypeID()) {
      // Trivial non-equality, e.g. PointerType and IntegerType
      return false;
    }

    if (ParamTy->isPointerTy()) {
      if (Param.hasByValAttr() !=
          CallSite->isByValArgument(ArgOp.getOperandNo())) {
        return false;
      }

      const auto *ParamSRetTy = Param.getParamStructRetType();
      const auto *ArgSRetTy =
          CallSite->getParamStructRetType(ArgOp.getOperandNo());
      if ((ParamSRetTy != nullptr) != (ArgSRetTy != nullptr)) {
        return false;
      }

      if (ParamSRetTy && ArgSRetTy) {
        // TODO: For better precision, compare the sret types as well
        // Trivial non-equality, e.g. PointerType and IntegerType
        if (ParamSRetTy->getTypeID() != ArgSRetTy->getTypeID()) {
          // Trivial non-equality, e.g. PointerType and IntegerType
          return false;
        }
      }
    }

    if (ParamTy->isStructTy()) {
      // Copied comment from struct-case in isTypeMatchForFunctionArgument():
      // > Well, we could do sanity checks here, but if the analysed code is
      // > insane we would miss callees, so we don't do that.

      continue;
    }

    // Types are non-equal and we could not find a reason to treat the same
    return false;
  }

  return true;
}
