/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/IntrinsicInst.h"

#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

static const llvm::Value *PoisonPill = reinterpret_cast<const llvm::Value *>(
    "all i need is a unique llvm::Value ptr...");

static const std::vector<const llvm::Value *> EmptySeq;
static const std::set<std::string> EmptyStringSet;

static std::string getTypeName(const llvm::Type *Type) {
  std::string TypeName;
  llvm::raw_string_ostream TypeRawOutputStream(TypeName);
  Type->print(TypeRawOutputStream);

  return TypeRawOutputStream.str();
}

static bool isMemoryLocationFrame(const llvm::Value *MemLocationPart) {
  return llvm::isa<llvm::AllocaInst>(MemLocationPart) ||
         llvm::isa<llvm::Argument>(MemLocationPart) ||
         llvm::isa<llvm::GlobalVariable>(MemLocationPart);
}

static bool isConstantIntEqual(const llvm::ConstantInt *CI1,
                               const llvm::ConstantInt *CI2) {
  // Compare numerical value without type
  // return ci1->getSExtValue() == ci2->getSExtValue();

  // Compare with type
  return CI1 == CI2;
}

static bool isGEPPartEqual(const llvm::GetElementPtrInst *MemLocationFactGEP,
                           const llvm::GetElementPtrInst *MemLocationInstGEP) {
  bool HaveValidGEPParts = MemLocationFactGEP->hasAllConstantIndices() &&
                           MemLocationInstGEP->hasAllConstantIndices();
  if (!HaveValidGEPParts) {
    return false;
  }

  bool IsNumIndicesEqual = MemLocationFactGEP->getNumIndices() ==
                           MemLocationInstGEP->getNumIndices();

  if (IsNumIndicesEqual) {
    // Compare pointer type
    auto *const GepFactPtrType = MemLocationFactGEP->getPointerOperandType();
    auto *const GepInstPtrType = MemLocationInstGEP->getPointerOperandType();
    if (GepFactPtrType != GepInstPtrType) {
      return false;
    }

    // Compare indices
    for (unsigned int I = 1; I < MemLocationFactGEP->getNumOperands(); I++) {
      const auto *GepFactIndex =
          llvm::cast<llvm::ConstantInt>(MemLocationFactGEP->getOperand(I));
      const auto *GepInstIndex =
          llvm::cast<llvm::ConstantInt>(MemLocationInstGEP->getOperand(I));

      if (!isConstantIntEqual(GepFactIndex, GepInstIndex)) {
        return false;
      }
    }
  } else {
    /*
     * For now just expect this to be the result of array decaying...
     *
     * If we pass an array as an argument it is decayed to a pointer and loses
     * type and size information. When we transfer the array from caller to
     * callee we copy the GEP instruction from the caller as this is the only
     * information we have. This GEP instruction carries type information:
     *
     * %arrayidx = getelementptr inbounds [42 x i32], [42 x i32]* %a, i64 0, i64
     * 5
     *
     * However every GEP instruction for that array in the callee refers to the
     * array as a pointer to the first element and performs pointer arithmetic
     * in order to step through the elements. Thus the same location in the
     * callee would be:
     *
     * %arrayidx = getelementptr inbounds i32, i32* %0, i64 5
     *
     * In order to be 100% accurate here we would also need to compare the
     * pointer types...
     */
    const auto *const NonDecayedArrayGEP =
        MemLocationFactGEP->getNumIndices() >
                MemLocationInstGEP->getNumIndices()
            ? MemLocationFactGEP
            : MemLocationInstGEP;

    if (auto *const NonDecayedArrayGEPPtrIndex =
            llvm::dyn_cast<llvm::ConstantInt>(
                NonDecayedArrayGEP->getOperand(1))) {
      if (!NonDecayedArrayGEPPtrIndex->isZero()) {
        return false;
      }
    } else {
      return false;
    }

    const auto *GepFactIndex =
        llvm::cast<llvm::ConstantInt>(MemLocationFactGEP->getOperand(
            MemLocationFactGEP->getNumOperands() - 1));
    const auto *GepInstIndex =
        llvm::cast<llvm::ConstantInt>(MemLocationInstGEP->getOperand(
            MemLocationInstGEP->getNumOperands() - 1));

    return isConstantIntEqual(GepFactIndex, GepInstIndex);
  }

  return true;
}

static bool isFirstNMemoryLocationPartsEqual(
    std::vector<const llvm::Value *> MemLocationSeqFact,
    std::vector<const llvm::Value *> MemLocationSeqInst, std::size_t N) {
  assert(N > 0);

  bool SeqsHaveAtLeastNParts =
      MemLocationSeqFact.size() >= N && MemLocationSeqInst.size() >= N;
  if (!SeqsHaveAtLeastNParts) {
    return false;
  }

  bool HaveMemLocationFrames =
      isMemoryLocationFrame(MemLocationSeqFact.front()) &&
      isMemoryLocationFrame(MemLocationSeqInst.front());
  if (!HaveMemLocationFrames) {
    return false;
  }

  static_assert(
      true,
      "We have vectors that both start with a memory location"
      "frame.Size may differ but we have at least n instances in each each.");

  bool IsSameMemLocationFrame =
      MemLocationSeqFact.front() == MemLocationSeqInst.front();
  if (!IsSameMemLocationFrame) {
    return false;
  }

  for (std::size_t I = 1; I < N; ++I) {
    const auto *const FactGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(MemLocationSeqFact[I]);
    const auto *const InstGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(MemLocationSeqInst[I]);

    bool IsEqual = isGEPPartEqual(FactGEPPtr, InstGEPPtr);
    if (!IsEqual) {
      return false;
    }
  }

  return true;
}

static bool isUnionBitCast(const llvm::CastInst *CastInst) {
  if (const auto *const BitCastInst =
          llvm::dyn_cast<llvm::BitCastInst>(CastInst)) {
    const auto TypeName = getTypeName(BitCastInst->getSrcTy());

    return TypeName.find("union") != std::string::npos;
  }
  return false;
}

static std::vector<const llvm::Value *>
getMemoryLocationSeqFromMatrRec(const llvm::Value *MemLocationPart) {
  // Globals
  if (const auto *const ConstExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(MemLocationPart)) {
    MemLocationPart =
        const_cast<llvm::ConstantExpr *>(ConstExpr)->getAsInstruction();
  }

  std::vector<const llvm::Value *> MemLocationSeq;

  bool IsMemLocationFrame = isMemoryLocationFrame(MemLocationPart);
  if (IsMemLocationFrame) {
    MemLocationSeq.push_back(MemLocationPart);

    return MemLocationSeq;
  }

  if (const auto *const CastInst =
          llvm::dyn_cast<llvm::CastInst>(MemLocationPart)) {
    MemLocationSeq = getMemoryLocationSeqFromMatrRec(CastInst->getOperand(0));

    bool PoisonSeq = isUnionBitCast(CastInst);
    if (!PoisonSeq) {
      return MemLocationSeq;
    }

    // FALLTHROUGH
  } else if (const auto *const LoadInst =
                 llvm::dyn_cast<llvm::LoadInst>(MemLocationPart)) {
    return getMemoryLocationSeqFromMatrRec(LoadInst->getOperand(0));
  } else if (const auto *const GepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(MemLocationPart)) {
    MemLocationSeq =
        getMemoryLocationSeqFromMatrRec(GepInst->getPointerOperand());

    bool IsSeqPoisoned =
        !MemLocationSeq.empty() && MemLocationSeq.back() == PoisonPill;
    if (IsSeqPoisoned) {
      return MemLocationSeq;
    }

    MemLocationSeq.push_back(GepInst);

    return MemLocationSeq;
  }

  // Poison seq
  bool IsSeqPoisoned =
      !MemLocationSeq.empty() && MemLocationSeq.back() == PoisonPill;
  if (!IsSeqPoisoned) {
    MemLocationSeq.push_back(PoisonPill);
  }

  return MemLocationSeq;
}

static std::vector<const llvm::Value *>
normalizeGlobalGEPs(const std::vector<const llvm::Value *> &MemLocationSeq) {
  bool IsGlobalMemLocationSeq =
      DataFlowUtils::isGlobalMemoryLocationSeq(MemLocationSeq);
  if (!IsGlobalMemLocationSeq) {
    return MemLocationSeq;
  }

  std::vector<const llvm::Value *> NormalizedMemLocationSeq;
  NormalizedMemLocationSeq.push_back(MemLocationSeq.front());

  for (std::size_t I = 1; I < MemLocationSeq.size(); ++I) {
    const auto *const GepInst =
        llvm::cast<llvm::GetElementPtrInst>(MemLocationSeq[I]);

    unsigned int NumIndices = GepInst->getNumIndices();

    bool IsNormalizedGEP = NumIndices <= 2;
    if (IsNormalizedGEP) {
      NormalizedMemLocationSeq.push_back(GepInst);
      continue;
    }

    const std::vector<llvm::Value *> Indices(GepInst->idx_begin(),
                                             GepInst->idx_end());

    auto *SplittedGEPInst = llvm::GetElementPtrInst::CreateInBounds(
        const_cast<llvm::Value *>(NormalizedMemLocationSeq.back()),
        {Indices[0], Indices[1]}, "gepsplit0");
    NormalizedMemLocationSeq.push_back(SplittedGEPInst);

    llvm::ConstantInt *ConstantZero = llvm::ConstantInt::get(
        GepInst->getType()->getContext(), llvm::APInt(32, 0, false));

    for (std::size_t I = 2; I < Indices.size(); ++I) {
      auto *const Index = Indices[I];

      std::stringstream NameStream;
      NameStream << "gepsplit" << (I - 1);

      SplittedGEPInst = llvm::GetElementPtrInst::CreateInBounds(
          const_cast<llvm::Value *>(NormalizedMemLocationSeq.back()),
          {ConstantZero, Index}, NameStream.str());
      NormalizedMemLocationSeq.push_back(SplittedGEPInst);
    }
  }

  return NormalizedMemLocationSeq;
}

static std::vector<const llvm::Value *>
normalizeMemoryLocationSeq(std::vector<const llvm::Value *> MemLocationSeq) {
  assert(!MemLocationSeq.empty());

  // Remove poison pill
  bool IsSeqPoisoned = MemLocationSeq.back() == PoisonPill;
  if (IsSeqPoisoned) {
    MemLocationSeq.pop_back();
  }

  if (MemLocationSeq.empty()) {
    return MemLocationSeq;
  }

  // Normalize global GEP parts
  MemLocationSeq = normalizeGlobalGEPs(MemLocationSeq);

  return MemLocationSeq;
}

std::vector<const llvm::Value *> DataFlowUtils::getMemoryLocationSeqFromMatr(
    const llvm::Value *MemLocationMatr) {
  auto MemLocationSeq = normalizeMemoryLocationSeq(
      getMemoryLocationSeqFromMatrRec(MemLocationMatr));

  assert(MemLocationSeq.empty() ||
         isMemoryLocationFrame(MemLocationSeq.front()));

  return MemLocationSeq;
}

std::vector<const llvm::Value *> DataFlowUtils::getMemoryLocationSeqFromFact(
    const ExtendedValue &MemLocationFact) {
  return MemLocationFact.getMemLocationSeq();
}

std::vector<const llvm::Value *>
DataFlowUtils::getVaListMemoryLocationSeqFromFact(
    const ExtendedValue &VaListFact) {
  return VaListFact.getVaListMemLocationSeq();
}

static const llvm::Value *
getMemoryLocationFrameFromFact(const ExtendedValue &MemLocationFact) {
  const auto MemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(MemLocationFact);
  if (MemLocationSeq.empty()) {
    return nullptr;
  }

  return MemLocationSeq.front();
}

static const llvm::Value *
getVaListMemoryLocationFrameFromFact(const ExtendedValue &VaListFact) {
  const auto MemLocationSeq =
      DataFlowUtils::getVaListMemoryLocationSeqFromFact(VaListFact);
  if (MemLocationSeq.empty()) {
    return nullptr;
  }

  return MemLocationSeq.front();
}

static const llvm::Value *
getMemoryLocationFrameFromMatr(const llvm::Value *MemLocationMatr) {
  const auto MemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(MemLocationMatr);
  if (MemLocationSeq.empty()) {
    return nullptr;
  }

  return MemLocationSeq.front();
}

bool DataFlowUtils::isValueTainted(const llvm::Value *CurrentInst,
                                   const ExtendedValue &Fact) {
  return CurrentInst == Fact.getValue();
}

bool DataFlowUtils::isMemoryLocationTainted(const llvm::Value *MemLocationMatr,
                                            const ExtendedValue &Fact) {
  auto MemLocationInstSeq = getMemoryLocationSeqFromMatr(MemLocationMatr);
  if (MemLocationInstSeq.empty()) {
    return false;
  }

  const auto MemLocationFactSeq = getMemoryLocationSeqFromFact(Fact);
  if (MemLocationFactSeq.empty()) {
    return false;
  }

  bool IsArrayDecay = DataFlowUtils::isArrayDecay(MemLocationMatr);
  if (IsArrayDecay) {
    MemLocationInstSeq.pop_back();
  }

  return isSubsetMemoryLocationSeq(MemLocationInstSeq, MemLocationFactSeq);
}

bool DataFlowUtils::isMemoryLocationSeqsEqual(
    const std::vector<const llvm::Value *> &MemLocationSeq1,
    const std::vector<const llvm::Value *> &MemLocationSeq2) {
  bool IsSizeEqual = MemLocationSeq1.size() == MemLocationSeq2.size();
  if (!IsSizeEqual) {
    return false;
  }

  bool IsEmptySeq = MemLocationSeq1.empty();
  if (IsEmptySeq) {
    return false;
  }

  std::size_t N = MemLocationSeq1.size();
  bool IsMemLocationsEqual =
      isFirstNMemoryLocationPartsEqual(MemLocationSeq1, MemLocationSeq2, N);
  if (!IsMemLocationsEqual) {
    return false;
  }

  return true;
}

bool DataFlowUtils::isSubsetMemoryLocationSeq(
    const std::vector<const llvm::Value *> &MemLocationSeqInst,
    const std::vector<const llvm::Value *> &MemLocationSeqFact) {
  if (MemLocationSeqInst.empty()) {
    return false;
  }
  if (MemLocationSeqFact.empty()) {
    return false;
  }

  std::size_t N = std::min<std::size_t>(MemLocationSeqInst.size(),
                                        MemLocationSeqFact.size());

  return isFirstNMemoryLocationPartsEqual(MemLocationSeqInst,
                                          MemLocationSeqFact, N);
}

std::vector<const llvm::Value *> DataFlowUtils::getRelocatableMemoryLocationSeq(
    const std::vector<const llvm::Value *> &TaintedMemLocationSeq,
    const std::vector<const llvm::Value *> &SrcMemLocationSeq) {
  std::vector<const llvm::Value *> RelocatableMemLocationSeq;

  for (std::size_t I = SrcMemLocationSeq.size();
       I < TaintedMemLocationSeq.size(); ++I) {
    RelocatableMemLocationSeq.push_back(TaintedMemLocationSeq[I]);
  }

  return RelocatableMemLocationSeq;
}

std::vector<const llvm::Value *> DataFlowUtils::joinMemoryLocationSeqs(
    const std::vector<const llvm::Value *> &MemLocationSeq1,
    const std::vector<const llvm::Value *> &MemLocationSeq2) {
  std::vector<const llvm::Value *> JoinedMemLocationSeq;
  JoinedMemLocationSeq.reserve(MemLocationSeq1.size() + MemLocationSeq2.size());

  JoinedMemLocationSeq.insert(JoinedMemLocationSeq.end(),
                              MemLocationSeq1.begin(), MemLocationSeq1.end());
  JoinedMemLocationSeq.insert(JoinedMemLocationSeq.end(),
                              MemLocationSeq2.begin(), MemLocationSeq2.end());

  return JoinedMemLocationSeq;
}

std::vector<const llvm::Value *>
getVaListMemoryLocationSeq(const llvm::Value *Value) {
  if (const auto *const PhiNodeInst = llvm::dyn_cast<llvm::PHINode>(Value)) {
    const auto PhiNodeName = PhiNodeInst->getName();
    bool IsVarArgAddr = PhiNodeName.contains_lower("vaarg.addr");
    if (!IsVarArgAddr) {
      return EmptySeq;
    }

    for (const auto &Block : PhiNodeInst->blocks()) {
      const auto BlockName = Block->getName();
      bool IsVarArgInMem = BlockName.contains_lower("vaarg.in_mem");
      if (!IsVarArgInMem) {
        continue;
      }

      auto *const VaListMemLocationMatr =
          PhiNodeInst->getIncomingValueForBlock(Block);
      const auto VaListMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(VaListMemLocationMatr);

      bool IsValidMemLocation = !VaListMemLocationSeq.empty();
      if (!IsValidMemLocation) {
        return EmptySeq;
      }

      return VaListMemLocationSeq;
    }
  }

  return EmptySeq;
}

static bool isArgumentEqual(const llvm::Value *SrcValue,
                            const ExtendedValue &Fact, bool IsVarArgFact) {
  const auto *const FactMemLocationFrame =
      IsVarArgFact ? getVaListMemoryLocationFrameFromFact(Fact)
                   : getMemoryLocationFrameFromFact(Fact);
  if (!FactMemLocationFrame) {
    return false;
  }

  if (const auto *const PatchableArgument =
          llvm::dyn_cast<llvm::Argument>(FactMemLocationFrame)) {
    if (PatchableArgument->hasByValAttr()) {
      return false;
    }

    if (const auto *const SrcValueArgument =
            llvm::dyn_cast<llvm::Argument>(SrcValue)) {
      bool IsLinkEqual = SrcValueArgument == PatchableArgument;
      if (IsLinkEqual) {
        return true;
      }
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableArgumentStore(const llvm::Value *SrcValue,
                                             const ExtendedValue &Fact) {
  bool IsVarArgFact = Fact.isVarArg();

  bool IsArgEqual = isArgumentEqual(SrcValue, Fact, IsVarArgFact);
  if (IsArgEqual) {
    return true;
  }

  /*
   * Patch of varargs passed through '...'
   */
  if (IsVarArgFact) {
    bool IsIndexEqual = Fact.getVarArgIndex() == Fact.getCurrentVarArgIndex();
    if (!IsIndexEqual) {
      return false;
    }

    if (const auto *const LoadInst = llvm::dyn_cast<llvm::LoadInst>(SrcValue)) {
      const auto *const PointerOperand = LoadInst->getPointerOperand();

      const auto VaListMemLocationSeq =
          getVaListMemoryLocationSeq(PointerOperand);
      bool IsValidMemLocation = !VaListMemLocationSeq.empty();
      if (!IsValidMemLocation) {
        return false;
      }

      return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                       VaListMemLocationSeq);
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableVaListArgument(const llvm::Value *SrcValue,
                                              const ExtendedValue &Fact) {
  bool IsVarArgFact = Fact.isVarArg();
  bool IsArgEqual = isArgumentEqual(SrcValue, Fact, IsVarArgFact);

  return IsVarArgFact && IsArgEqual;
}

bool DataFlowUtils::isPatchableArgumentMemcpy(
    const llvm::Value *SrcValue,
    const std::vector<const llvm::Value *> &SrcMemLocationSeq,
    const ExtendedValue &Fact) {
  bool IsVarArgFact = Fact.isVarArg();
  if (!IsVarArgFact) {
    return false;
  }

  bool IsIndexEqual = Fact.getVarArgIndex() == Fact.getCurrentVarArgIndex();
  if (!IsIndexEqual) {
    return false;
  }

  bool IsSrcMemLocation = !SrcMemLocationSeq.empty();
  if (IsSrcMemLocation) {

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                     SrcMemLocationSeq);
  } else if (const auto *const BitCastInst =
                 llvm::dyn_cast<llvm::BitCastInst>(SrcValue)) {
    auto *const PointerOperand = BitCastInst->getOperand(0);

    const auto VaListMemLocationSeq =
        getVaListMemoryLocationSeq(PointerOperand);
    bool IsValidMemLocation = !VaListMemLocationSeq.empty();
    if (!IsValidMemLocation) {
      return false;
    }

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                     VaListMemLocationSeq);
  }

  return false;
}

bool DataFlowUtils::isPatchableReturnValue(const llvm::Value *SrcValue,
                                           const ExtendedValue &Fact) {
  /*
   * We could also check against the fact which is also a call inst when we
   * have a return value. However as we are not changing the fact after
   * relocation it would be again taken into account. If we use the patch
   * part it is gone after first patch.
   */
  const auto *const FactMemLocationFrame = getMemoryLocationFrameFromFact(Fact);
  if (!FactMemLocationFrame) {
    return false;
  }

  if (const auto *const PatchableCallInst =
          llvm::dyn_cast<llvm::CallInst>(FactMemLocationFrame)) {

    if (const auto *const SrcValueExtractValueInst =
            llvm::dyn_cast<llvm::ExtractValueInst>(SrcValue)) {
      bool IsLinkEqual =
          SrcValueExtractValueInst->getAggregateOperand() == PatchableCallInst;
      if (IsLinkEqual) {
        return true;
      }
    } else if (const auto *const SrcValueCallInst =
                   llvm::dyn_cast<llvm::CallInst>(SrcValue)) {
      bool IsLinkEqual = SrcValueCallInst == PatchableCallInst;
      if (IsLinkEqual) {
        return true;
      }
    }
  }

  return false;
}

std::vector<const llvm::Value *> DataFlowUtils::patchMemoryLocationFrame(
    const std::vector<const llvm::Value *> &PatchableMemLocationSeq,
    const std::vector<const llvm::Value *> &PatchMemLocationSeq) {
  if (PatchableMemLocationSeq.empty()) {
    return EmptySeq;
  }
  if (PatchMemLocationSeq.empty()) {
    return EmptySeq;
  }

  std::vector<const llvm::Value *> PatchedMemLocationSeq;
  PatchedMemLocationSeq.reserve((PatchableMemLocationSeq.size() - 1) +
                                PatchMemLocationSeq.size());

  PatchedMemLocationSeq.insert(PatchedMemLocationSeq.end(),
                               PatchMemLocationSeq.begin(),
                               PatchMemLocationSeq.end());
  PatchedMemLocationSeq.insert(PatchedMemLocationSeq.end(),
                               std::next(PatchableMemLocationSeq.begin()),
                               PatchableMemLocationSeq.end());

  return PatchedMemLocationSeq;
}

static long getNumCoercedArgs(const llvm::Value *Value) {
  if (const auto *const ConstExpr = llvm::dyn_cast<llvm::ConstantExpr>(Value)) {
    Value = const_cast<llvm::ConstantExpr *>(ConstExpr)->getAsInstruction();
  }

  if (llvm::isa<llvm::AllocaInst>(Value) ||
      llvm::isa<llvm::GlobalVariable>(Value)) {
    return -4711;
  }

  if (const auto *const BitCastInst =
          llvm::dyn_cast<llvm::BitCastInst>(Value)) {
    long Ret = getNumCoercedArgs(BitCastInst->getOperand(0));

    if (Ret == -4711) {
      auto *const DstType = BitCastInst->getDestTy();
      if (!DstType->isPointerTy()) {
        return -1;
      }

      auto *const ElementType = DstType->getPointerElementType();

      if (auto *const StructType =
              llvm::dyn_cast<llvm::StructType>(ElementType)) {
        return static_cast<long>(StructType->getNumElements());
      }
      return -1;
    }

    return Ret;
  } else if (const auto *const GepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(Value)) {
    return getNumCoercedArgs(GepInst->getPointerOperand());
  } else if (const auto *const LoadInst =
                 llvm::dyn_cast<llvm::LoadInst>(Value)) {
    return getNumCoercedArgs(LoadInst->getPointerOperand());
  }

  return -1;
}

/*
 * The purpose of this function is to provide a sanitized arg list.
 * Sanitization comprises the following 2 steps:
 *
 * (1) Only keep one coerced argument and fix mem location sequence.
 *     This is extremely important when using varargs as we would
 *     increment the var args index for every coerced element although
 *     we only need one index for the struct. The way we figure out the
 *     amount of coerced args for a struct is to retrieve the bitcast
 *     and count its members. Notes for fixing the mem location sequence
 *     can be found below.
 *
 * (2) Provide a default formal parameter for varargs
 *
 * If the struct is coerced then the indexes are not matching anymore.
 * E.g. if we have the following struct:
 *
 * struct s1 {
 *   int a;
 *   int b;
 *   char *t1;
 * };
 *
 * If we taint t1 we will have Alloca_x -> GEP 2 as our memory location.
 *
 * Now if a and b are coerced from i32, i32 to i64 we will have a struct
 * that only contains two members (i64, i8*). This means that also the
 * GEP indexes are different (there is no GEP 2 anymore). So we just ignore
 * the GEP value and pop it from the memory location and proceed as usual.
 */
std::vector<
    std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
               const llvm::Value *>>
DataFlowUtils::getSanitizedArgList(const llvm::CallInst *CallInst,
                                   const llvm::Function *DestFun,
                                   const llvm::Value *ZeroValue) {
  std::vector<
      std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
                 const llvm::Value *>>
      SanitizedArgList;

  for (unsigned I = 0; I < CallInst->getNumArgOperands(); ++I) {
    auto *const Arg = CallInst->getOperand(I);
    const auto *const Param = getNthFunctionArgument(DestFun, I);

    auto ArgMemLocationSeq = DataFlowUtils::getMemoryLocationSeqFromMatr(Arg);

    long NumCoersedArgs = getNumCoercedArgs(Arg);
    bool IsCoersedArg = NumCoersedArgs > 0;

    bool IsArrayDecay = DataFlowUtils::isArrayDecay(Arg);

    if (IsCoersedArg) {
      ArgMemLocationSeq.pop_back();
      I += NumCoersedArgs - 1;
    } else if (IsArrayDecay) {
      ArgMemLocationSeq.pop_back();
    }

    const auto *const SanitizedParam = Param ? Param : ZeroValue;

    SanitizedArgList.emplace_back(Arg, ArgMemLocationSeq, SanitizedParam);
  }

  return SanitizedArgList;
}

static std::vector<llvm::BasicBlock *> getPostDominators(
    const llvm::DomTreeNodeBase<llvm::BasicBlock> *PostDomTreeNode,
    const llvm::BasicBlock *StartBasicBlock) {
  auto *const CurrentBasicBlock = PostDomTreeNode->getBlock();
  bool IsStartBasicBlock = CurrentBasicBlock == StartBasicBlock;

  if (IsStartBasicBlock) {
    return {CurrentBasicBlock};
  }

  for (auto *const PostDomTreeChild : PostDomTreeNode->getChildren()) {
    auto ChildNodes = getPostDominators(PostDomTreeChild, StartBasicBlock);
    if (!ChildNodes.empty()) {
      ChildNodes.push_back(CurrentBasicBlock);

      return ChildNodes;
    }
  }

  return std::vector<llvm::BasicBlock *>();
}

const llvm::BasicBlock *
DataFlowUtils::getEndOfTaintedBlock(const llvm::BasicBlock *StartBasicBlock) {
  const auto *const TerminatorInst = StartBasicBlock->getTerminator();
  auto *const Function =
      const_cast<llvm::Function *>(StartBasicBlock->getParent());

  bool IsBlockStatement = llvm::isa<llvm::BranchInst>(TerminatorInst) ||
                          llvm::isa<llvm::SwitchInst>(TerminatorInst);
  if (!IsBlockStatement) {
    return nullptr;
  }

  llvm::PostDominatorTree PostDominatorTree;
  PostDominatorTree.recalculate(*Function);

  const auto PostDominators =
      getPostDominators(PostDominatorTree.getRootNode(), StartBasicBlock);

  return PostDominators.size() > 1 ? PostDominators[1] : nullptr;
}

/*
 * We are removing the tainted branch instruction from facts if the
 * instruction's basic block label matches the one of the tainted branch end
 * block. Note that we remove it after the phi node making sure that the phi
 * node is auto added whenever we came from a tainted branch.
 */
bool DataFlowUtils::removeTaintedBlockInst(
    const ExtendedValue &Fact, const llvm::Instruction *CurrentInst) {
  bool IsEndOfFunctionTaint = Fact.getEndOfTaintedBlockLabel().empty();
  if (IsEndOfFunctionTaint) {
    return false;
  }

  bool IsPhiNode = llvm::isa<llvm::PHINode>(CurrentInst);
  if (IsPhiNode) {
    return false;
  }

  const auto *const CurrentBB = CurrentInst->getParent();
  const auto CurrentLabel = CurrentBB->getName();

  return CurrentLabel == Fact.getEndOfTaintedBlockLabel();
}

bool DataFlowUtils::isAutoGENInTaintedBlock(
    const llvm::Instruction *CurrentInst) {
  return !llvm::isa<llvm::StoreInst>(CurrentInst) &&
         !llvm::isa<llvm::MemTransferInst>(CurrentInst) &&
         !llvm::isa<llvm::BranchInst>(CurrentInst) &&
         !llvm::isa<llvm::SwitchInst>(CurrentInst) &&
         !llvm::isa<llvm::ReturnInst>(CurrentInst);
}

bool DataFlowUtils::isMemoryLocationFact(const ExtendedValue &EV) {
  return !EV.getMemLocationSeq().empty();
}

bool DataFlowUtils::isKillAfterStoreFact(const ExtendedValue &EV) {
  return !isMemoryLocationFact(EV) && !llvm::isa<llvm::CallInst>(EV.getValue());
}

bool DataFlowUtils::isCheckOperandsInst(const llvm::Instruction *CurrentInst) {
  bool IsLoad = llvm::isa<llvm::LoadInst>(CurrentInst);
  if (IsLoad) {
    return false;
  }

  return llvm::isa<llvm::UnaryInstruction>(CurrentInst) ||
         llvm::isa<llvm::BinaryOperator>(CurrentInst) ||
         llvm::isa<llvm::CmpInst>(CurrentInst) ||
         llvm::isa<llvm::SelectInst>(CurrentInst);
}

bool DataFlowUtils::isAutoIdentity(const llvm::Instruction *CurrentInst,
                                   const ExtendedValue &Fact) {
  bool IsVarArgTemplate = Fact.isVarArgTemplate();
  if (IsVarArgTemplate) {
    return !llvm::isa<llvm::VAStartInst>(CurrentInst);
  }

  /*
   * If we are dealing with varargs we need to make sure that the internal
   * structure va_list is never tainted (not even in an auto taint scenario).
   * This would lead to detecting conditions as tainted for varargs internal
   * processing branches which further leads to auto tainting of every vararg.
   * For traceability disable this check and run test case
   * 200-map-to-callee-varargs-15. The interesting part begins at line 101 in
   * the IR.
   */
  if (const auto *const StoreInst =
          llvm::dyn_cast<llvm::StoreInst>(CurrentInst)) {
    const auto *const SrcMemLocationMatr = StoreInst->getValueOperand();
    const auto *const SrcMemLocationFrame =
        getMemoryLocationFrameFromMatr(SrcMemLocationMatr);

    bool IsArgumentPatch =
        llvm::isa_and_nonnull<llvm::Argument>(SrcMemLocationFrame);
    if (IsArgumentPatch) {
      return false;
    }

    const auto *const DstMemLocationMatr = StoreInst->getPointerOperand();
    const auto DstMemLocationSeq =
        getMemoryLocationSeqFromMatr(DstMemLocationMatr);

    bool IsDstMemLocation = !DstMemLocationSeq.empty();
    if (IsDstMemLocation) {
      auto *const MemLocationFrameType = DstMemLocationSeq.front()->getType();

      bool IsMemLocationFrameTypeVaList = isVaListType(MemLocationFrameType);
      if (IsMemLocationFrameTypeVaList) {
        return true;
      }
    }
  }

  return false;
}

bool DataFlowUtils::isVarArgParam(const llvm::Value *Param,
                                  const llvm::Value *ZeroValue) {
  return Param == ZeroValue;
}

bool DataFlowUtils::isVaListType(const llvm::Type *Type) {
  const auto TypeName = getTypeName(Type);

  return TypeName.find("%struct.__va_list_tag") != std::string::npos;
}

bool DataFlowUtils::isReturnValue(const llvm::Instruction *CurrentInst,
                                  const llvm::Instruction *SuccessorInst) {
  bool IsSuccessorRetVal = llvm::isa<llvm::ReturnInst>(SuccessorInst);
  if (!IsSuccessorRetVal) {
    return false;
  }

  if (const auto *const BinaryOpInst =
          llvm::dyn_cast<llvm::BinaryOperator>(CurrentInst)) {
    bool IsMagicOpCode = BinaryOpInst->getOpcode() == 20;
    if (!IsMagicOpCode) {
      return false;
    }

    bool IsMagicType = getTypeName(BinaryOpInst->getType()) == "i4711";
    if (!IsMagicType) {
      return false;
    }

    return true;
  }

  return false;
}

/*
 * We use the following conditions to check whether a memory location is an
 * array decay or not:
 *
 * (1) Last GEP is of type array
 * (2) There is no load after our last GEP array
 *
 * Check 071-arrays-3, 071-arrays-11, 200-map-to-callee-variable-array-2,
 * 200-map-to-callee-varargs-30, 260-globals-12
 */
bool DataFlowUtils::isArrayDecay(const llvm::Value *MemLocationMatr) {
  if (!MemLocationMatr) {
    return false;
  }

  if (const auto *const ConstExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(MemLocationMatr)) {
    MemLocationMatr =
        const_cast<llvm::ConstantExpr *>(ConstExpr)->getAsInstruction();
  }

  bool IsMemLocationFrame = isMemoryLocationFrame(MemLocationMatr);
  if (IsMemLocationFrame) {
    return false;
  } else if (const auto *const CastInst =
                 llvm::dyn_cast<llvm::CastInst>(MemLocationMatr)) {
    return isArrayDecay(CastInst->getOperand(0));
  } else if (const auto *const GepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(MemLocationMatr)) {
    bool IsSrcMemLocationArrayType =
        GepInst->getPointerOperandType()->getPointerElementType()->isArrayTy();
    if (IsSrcMemLocationArrayType) {
      return true;
    }

    return false;
  } else if (const auto *const LoadInst =
                 llvm::dyn_cast<llvm::LoadInst>(MemLocationMatr)) {
    return false;
  }

  return false;
}

bool DataFlowUtils::isGlobalMemoryLocationSeq(
    const std::vector<const llvm::Value *> &MemLocationSeq) {
  if (MemLocationSeq.empty()) {
    return false;
  }

  return llvm::isa<llvm::GlobalVariable>(MemLocationSeq.front());
}

static void
dumpMemoryLocation(const std::vector<const llvm::Value *> &MemLocationSeq) {
#ifdef DEBUG_BUILD
  for (const auto MemLocationPart : MemLocationSeq) {
    llvm::outs() << "[ENV_TRACE] ";
    MemLocationPart->print(llvm::outs());
    llvm::outs() << "\n";
    llvm::outs().flush();
  }
#endif
}

void DataFlowUtils::dumpFact(const ExtendedValue &EV) {
  if (!EV.getMemLocationSeq().empty()) {
    LOG_DEBUG("memLocationSeq:");
    dumpMemoryLocation(EV.getMemLocationSeq());
  }

  if (!EV.getEndOfTaintedBlockLabel().empty()) {
    LOG_DEBUG("endOfTaintedBlockLabel: " << EV.getEndOfTaintedBlockLabel());
  }

  if (EV.isVarArg()) {
    if (!EV.isVarArgTemplate()) {
      LOG_DEBUG("vaListMemLocationSeq:");
      dumpMemoryLocation(getVaListMemoryLocationSeqFromFact(EV));
    }
    LOG_DEBUG("varArgIndex: " << EV.getVarArgIndex());
    LOG_DEBUG("currentVarArgIndex: " << EV.getCurrentVarArgIndex());
  }
}

static std::set<std::string> readFileFromEnvVar(const char *EnvVar) {
  std::set<std::string> Lines;

  const char *FilePath = std::getenv(EnvVar);
  if (!FilePath) {
    LOG_INFO(EnvVar << " unset");
    return Lines;
  } else {
    LOG_INFO(EnvVar << " set to: " << FilePath);
  }

  std::ifstream Fis(FilePath);
  if (Fis.fail()) {
    LOG_INFO("Failed to read from: " << FilePath);
    return Lines;
  }

  std::string Line;
  while (std::getline(Fis, Line)) {
    if (Line.empty()) {
      continue;
    }
    if (Line.at(0) == '#') {
      continue;
    }

    Lines.insert(Line);
  }

  return Lines;
}

std::set<std::string> DataFlowUtils::getTaintedFunctions() {
  std::set<std::string> TaintedFunctions =
      readFileFromEnvVar("TAINTED_FUNCTIONS_LOCATION");
  if (TaintedFunctions.empty()) {
    TaintedFunctions = {"getenv", "secure_getenv"};
  }

  LOG_INFO("Tainted functions:");
  for (const auto &TaintedFunction : TaintedFunctions) {
    LOG_INFO(TaintedFunction);
  }

  return TaintedFunctions;
}

std::set<std::string> DataFlowUtils::getBlacklistedFunctions() {
  std::set<std::string> BlacklistedFunctions =
      readFileFromEnvVar("BLACKLISTED_FUNCTIONS_LOCATION");
  if (BlacklistedFunctions.empty()) {
    BlacklistedFunctions = {"printf"};
  }

  LOG_INFO("Blacklisted functions:");
  for (const auto &BlacklistedFunction : BlacklistedFunctions) {
    LOG_INFO(BlacklistedFunction);
  }

  return BlacklistedFunctions;
}

std::string
DataFlowUtils::getTraceFilenamePrefix(const std::string &EntryPoint) {
  time_t Time = std::time(nullptr);
  long Now = static_cast<long>(Time);

  std::stringstream TraceFileStream;
  TraceFileStream << "static"
                  << "-" << EntryPoint << "-" << Now;

  return TraceFileStream.str();
}
