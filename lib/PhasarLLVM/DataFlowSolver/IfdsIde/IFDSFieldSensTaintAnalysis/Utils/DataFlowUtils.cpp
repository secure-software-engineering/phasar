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

static const llvm::Value *POISON_PILL = reinterpret_cast<const llvm::Value *>(
    "all i need is a unique llvm::Value ptr...");

static const std::vector<const llvm::Value *> EMPTY_SEQ;
static const std::set<std::string> EMPTY_STRING_SET;

static const std::string getTypeName(const llvm::Type *Type) {
  std::string typeName;
  llvm::raw_string_ostream typeRawOutputStream(typeName);
  Type->print(typeRawOutputStream);

  return typeRawOutputStream.str();
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
  bool haveValidGEPParts = MemLocationFactGEP->hasAllConstantIndices() &&
                           MemLocationInstGEP->hasAllConstantIndices();
  if (!haveValidGEPParts)
    return false;

  bool isNumIndicesEqual = MemLocationFactGEP->getNumIndices() ==
                           MemLocationInstGEP->getNumIndices();

  if (isNumIndicesEqual) {
    // Compare pointer type
    const auto gepFactPtrType = MemLocationFactGEP->getPointerOperandType();
    const auto gepInstPtrType = MemLocationInstGEP->getPointerOperandType();
    if (gepFactPtrType != gepInstPtrType)
      return false;

    // Compare indices
    for (unsigned int i = 1; i < MemLocationFactGEP->getNumOperands(); i++) {
      const auto *gepFactIndex =
          llvm::cast<llvm::ConstantInt>(MemLocationFactGEP->getOperand(i));
      const auto *gepInstIndex =
          llvm::cast<llvm::ConstantInt>(MemLocationInstGEP->getOperand(i));

      if (!isConstantIntEqual(gepFactIndex, gepInstIndex))
        return false;
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
    const auto nonDecayedArrayGEP = MemLocationFactGEP->getNumIndices() >
                                            MemLocationInstGEP->getNumIndices()
                                        ? MemLocationFactGEP
                                        : MemLocationInstGEP;

    if (const auto nonDecayedArrayGEPPtrIndex =
            llvm::dyn_cast<llvm::ConstantInt>(
                nonDecayedArrayGEP->getOperand(1))) {
      if (!nonDecayedArrayGEPPtrIndex->isZero())
        return false;
    } else {
      return false;
    }

    const auto *gepFactIndex =
        llvm::cast<llvm::ConstantInt>(MemLocationFactGEP->getOperand(
            MemLocationFactGEP->getNumOperands() - 1));
    const auto *gepInstIndex =
        llvm::cast<llvm::ConstantInt>(MemLocationInstGEP->getOperand(
            MemLocationInstGEP->getNumOperands() - 1));

    return isConstantIntEqual(gepFactIndex, gepInstIndex);
  }

  return true;
}

static bool isFirstNMemoryLocationPartsEqual(
    std::vector<const llvm::Value *> MemLocationSeqFact,
    std::vector<const llvm::Value *> MemLocationSeqInst, std::size_t N) {
  assert(N > 0);

  bool seqsHaveAtLeastNParts =
      MemLocationSeqFact.size() >= N && MemLocationSeqInst.size() >= N;
  if (!seqsHaveAtLeastNParts)
    return false;

  bool haveMemLocationFrames =
      isMemoryLocationFrame(MemLocationSeqFact.front()) &&
      isMemoryLocationFrame(MemLocationSeqInst.front());
  if (!haveMemLocationFrames)
    return false;

  static_assert(
      true,
      "We have vectors that both start with a memory location"
      "frame.Size may differ but we have at least n instances in each each.");

  bool isSameMemLocationFrame =
      MemLocationSeqFact.front() == MemLocationSeqInst.front();
  if (!isSameMemLocationFrame)
    return false;

  for (std::size_t i = 1; i < N; ++i) {
    const auto factGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(MemLocationSeqFact[i]);
    const auto instGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(MemLocationSeqInst[i]);

    bool isEqual = isGEPPartEqual(factGEPPtr, instGEPPtr);
    if (!isEqual)
      return false;
  }

  return true;
}

static bool isUnionBitCast(const llvm::CastInst *CastInst) {
  if (const auto bitCastInst = llvm::dyn_cast<llvm::BitCastInst>(CastInst)) {
    const auto typeName = getTypeName(bitCastInst->getSrcTy());

    return typeName.find("union") != std::string::npos;
  }
  return false;
}

static std::vector<const llvm::Value *>
getMemoryLocationSeqFromMatrRec(const llvm::Value *MemLocationPart) {
  // Globals
  if (const auto constExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(MemLocationPart)) {
    MemLocationPart =
        const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  std::vector<const llvm::Value *> memLocationSeq;

  bool isMemLocationFrame = isMemoryLocationFrame(MemLocationPart);
  if (isMemLocationFrame) {
    memLocationSeq.push_back(MemLocationPart);

    return memLocationSeq;
  }

  if (const auto castInst = llvm::dyn_cast<llvm::CastInst>(MemLocationPart)) {
    memLocationSeq = getMemoryLocationSeqFromMatrRec(castInst->getOperand(0));

    bool poisonSeq = isUnionBitCast(castInst);
    if (!poisonSeq)
      return memLocationSeq;

    // FALLTHROUGH
  } else if (const auto loadInst =
                 llvm::dyn_cast<llvm::LoadInst>(MemLocationPart)) {
    return getMemoryLocationSeqFromMatrRec(loadInst->getOperand(0));
  } else if (const auto gepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(MemLocationPart)) {
    memLocationSeq =
        getMemoryLocationSeqFromMatrRec(gepInst->getPointerOperand());

    bool isSeqPoisoned =
        !memLocationSeq.empty() && memLocationSeq.back() == POISON_PILL;
    if (isSeqPoisoned)
      return memLocationSeq;

    memLocationSeq.push_back(gepInst);

    return memLocationSeq;
  }

  // Poison seq
  bool isSeqPoisoned =
      !memLocationSeq.empty() && memLocationSeq.back() == POISON_PILL;
  if (!isSeqPoisoned)
    memLocationSeq.push_back(POISON_PILL);

  return memLocationSeq;
}

static const std::vector<const llvm::Value *>
normalizeGlobalGEPs(const std::vector<const llvm::Value *> MemLocationSeq) {
  bool isGlobalMemLocationSeq =
      DataFlowUtils::isGlobalMemoryLocationSeq(MemLocationSeq);
  if (!isGlobalMemLocationSeq)
    return MemLocationSeq;

  std::vector<const llvm::Value *> normalizedMemLocationSeq;
  normalizedMemLocationSeq.push_back(MemLocationSeq.front());

  for (std::size_t i = 1; i < MemLocationSeq.size(); ++i) {
    const auto gepInst = llvm::cast<llvm::GetElementPtrInst>(MemLocationSeq[i]);

    unsigned int numIndices = gepInst->getNumIndices();

    bool isNormalizedGEP = numIndices <= 2;
    if (isNormalizedGEP) {
      normalizedMemLocationSeq.push_back(gepInst);
      continue;
    }

    const std::vector<llvm::Value *> indices(gepInst->idx_begin(),
                                             gepInst->idx_end());

    auto splittedGEPInst = llvm::GetElementPtrInst::CreateInBounds(
        const_cast<llvm::Value *>(normalizedMemLocationSeq.back()),
        {indices[0], indices[1]}, "gepsplit0");
    normalizedMemLocationSeq.push_back(splittedGEPInst);

    llvm::ConstantInt *constantZero = llvm::ConstantInt::get(
        gepInst->getType()->getContext(), llvm::APInt(32, 0, false));

    for (std::size_t i = 2; i < indices.size(); ++i) {
      const auto index = indices[i];

      std::stringstream nameStream;
      nameStream << "gepsplit" << (i - 1);

      splittedGEPInst = llvm::GetElementPtrInst::CreateInBounds(
          const_cast<llvm::Value *>(normalizedMemLocationSeq.back()),
          {constantZero, index}, nameStream.str());
      normalizedMemLocationSeq.push_back(splittedGEPInst);
    }
  }

  return normalizedMemLocationSeq;
}

static std::vector<const llvm::Value *>
normalizeMemoryLocationSeq(std::vector<const llvm::Value *> MemLocationSeq) {
  assert(!MemLocationSeq.empty());

  // Remove poison pill
  bool isSeqPoisoned = MemLocationSeq.back() == POISON_PILL;
  if (isSeqPoisoned)
    MemLocationSeq.pop_back();

  if (MemLocationSeq.empty())
    return MemLocationSeq;

  // Normalize global GEP parts
  MemLocationSeq = normalizeGlobalGEPs(MemLocationSeq);

  return MemLocationSeq;
}

const std::vector<const llvm::Value *>
DataFlowUtils::getMemoryLocationSeqFromMatr(
    const llvm::Value *MemLocationMatr) {
  auto memLocationSeq = normalizeMemoryLocationSeq(
      getMemoryLocationSeqFromMatrRec(MemLocationMatr));

  assert(memLocationSeq.empty() ||
         isMemoryLocationFrame(memLocationSeq.front()));

  return memLocationSeq;
}

const std::vector<const llvm::Value *>
DataFlowUtils::getMemoryLocationSeqFromFact(
    const ExtendedValue &MemLocationFact) {
  return MemLocationFact.getMemLocationSeq();
}

const std::vector<const llvm::Value *>
DataFlowUtils::getVaListMemoryLocationSeqFromFact(
    const ExtendedValue &VaListFact) {
  return VaListFact.getVaListMemLocationSeq();
}

static const llvm::Value *
getMemoryLocationFrameFromFact(const ExtendedValue &MemLocationFact) {
  const auto memLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(MemLocationFact);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

static const llvm::Value *
getVaListMemoryLocationFrameFromFact(const ExtendedValue &VaListFact) {
  const auto memLocationSeq =
      DataFlowUtils::getVaListMemoryLocationSeqFromFact(VaListFact);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

static const llvm::Value *
getMemoryLocationFrameFromMatr(const llvm::Value *MemLocationMatr) {
  const auto memLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(MemLocationMatr);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

bool DataFlowUtils::isValueTainted(const llvm::Value *CurrentInst,
                                   const ExtendedValue &Fact) {
  return CurrentInst == Fact.getValue();
}

bool DataFlowUtils::isMemoryLocationTainted(const llvm::Value *MemLocationMatr,
                                            const ExtendedValue &Fact) {
  auto memLocationInstSeq = getMemoryLocationSeqFromMatr(MemLocationMatr);
  if (memLocationInstSeq.empty())
    return false;

  const auto memLocationFactSeq = getMemoryLocationSeqFromFact(Fact);
  if (memLocationFactSeq.empty())
    return false;

  bool isArrayDecay = DataFlowUtils::isArrayDecay(MemLocationMatr);
  if (isArrayDecay)
    memLocationInstSeq.pop_back();

  return isSubsetMemoryLocationSeq(memLocationInstSeq, memLocationFactSeq);
}

bool DataFlowUtils::isMemoryLocationSeqsEqual(
    const std::vector<const llvm::Value *> MemLocationSeq1,
    const std::vector<const llvm::Value *> MemLocationSeq2) {
  bool isSizeEqual = MemLocationSeq1.size() == MemLocationSeq2.size();
  if (!isSizeEqual)
    return false;

  bool isEmptySeq = MemLocationSeq1.empty();
  if (isEmptySeq)
    return false;

  std::size_t n = MemLocationSeq1.size();
  bool isMemLocationsEqual =
      isFirstNMemoryLocationPartsEqual(MemLocationSeq1, MemLocationSeq2, n);
  if (!isMemLocationsEqual)
    return false;

  return true;
}

bool DataFlowUtils::isSubsetMemoryLocationSeq(
    const std::vector<const llvm::Value *> MemLocationSeqInst,
    const std::vector<const llvm::Value *> MemLocationSeqFact) {
  if (MemLocationSeqInst.empty())
    return false;
  if (MemLocationSeqFact.empty())
    return false;

  std::size_t n = std::min<std::size_t>(MemLocationSeqInst.size(),
                                        MemLocationSeqFact.size());

  return isFirstNMemoryLocationPartsEqual(MemLocationSeqInst,
                                          MemLocationSeqFact, n);
}

const std::vector<const llvm::Value *>
DataFlowUtils::getRelocatableMemoryLocationSeq(
    const std::vector<const llvm::Value *> TaintedMemLocationSeq,
    const std::vector<const llvm::Value *> SrcMemLocationSeq) {
  std::vector<const llvm::Value *> relocatableMemLocationSeq;

  for (std::size_t i = SrcMemLocationSeq.size();
       i < TaintedMemLocationSeq.size(); ++i) {
    relocatableMemLocationSeq.push_back(TaintedMemLocationSeq[i]);
  }

  return relocatableMemLocationSeq;
}

const std::vector<const llvm::Value *> DataFlowUtils::joinMemoryLocationSeqs(
    const std::vector<const llvm::Value *> MemLocationSeq1,
    const std::vector<const llvm::Value *> MemLocationSeq2) {
  std::vector<const llvm::Value *> joinedMemLocationSeq;
  joinedMemLocationSeq.reserve(MemLocationSeq1.size() + MemLocationSeq2.size());

  joinedMemLocationSeq.insert(joinedMemLocationSeq.end(),
                              MemLocationSeq1.begin(), MemLocationSeq1.end());
  joinedMemLocationSeq.insert(joinedMemLocationSeq.end(),
                              MemLocationSeq2.begin(), MemLocationSeq2.end());

  return joinedMemLocationSeq;
}

const std::vector<const llvm::Value *>
getVaListMemoryLocationSeq(const llvm::Value *Value) {
  if (const auto phiNodeInst = llvm::dyn_cast<llvm::PHINode>(Value)) {
    const auto phiNodeName = phiNodeInst->getName();
    bool isVarArgAddr = phiNodeName.contains_lower("vaarg.addr");
    if (!isVarArgAddr)
      return EMPTY_SEQ;

    for (const auto &block : phiNodeInst->blocks()) {
      const auto blockName = block->getName();
      bool isVarArgInMem = blockName.contains_lower("vaarg.in_mem");
      if (!isVarArgInMem)
        continue;

      const auto vaListMemLocationMatr =
          phiNodeInst->getIncomingValueForBlock(block);
      const auto vaListMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(vaListMemLocationMatr);

      bool isValidMemLocation = !vaListMemLocationSeq.empty();
      if (!isValidMemLocation)
        return EMPTY_SEQ;

      return vaListMemLocationSeq;
    }
  }

  return EMPTY_SEQ;
}

static bool isArgumentEqual(const llvm::Value *SrcValue,
                            const ExtendedValue &Fact, bool IsVarArgFact) {
  const auto factMemLocationFrame =
      IsVarArgFact ? getVaListMemoryLocationFrameFromFact(Fact)
                   : getMemoryLocationFrameFromFact(Fact);
  if (!factMemLocationFrame)
    return false;

  if (const auto patchableArgument =
          llvm::dyn_cast<llvm::Argument>(factMemLocationFrame)) {
    if (patchableArgument->hasByValAttr())
      return false;

    if (const auto srcValueArgument =
            llvm::dyn_cast<llvm::Argument>(SrcValue)) {
      bool isLinkEqual = srcValueArgument == patchableArgument;
      if (isLinkEqual)
        return true;
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableArgumentStore(const llvm::Value *SrcValue,
                                             const ExtendedValue &Fact) {
  bool isVarArgFact = Fact.isVarArg();

  bool isArgEqual = isArgumentEqual(SrcValue, Fact, isVarArgFact);
  if (isArgEqual)
    return true;

  /*
   * Patch of varargs passed through '...'
   */
  if (isVarArgFact) {
    bool isIndexEqual = Fact.getVarArgIndex() == Fact.getCurrentVarArgIndex();
    if (!isIndexEqual)
      return false;

    if (const auto loadInst = llvm::dyn_cast<llvm::LoadInst>(SrcValue)) {
      const auto pointerOperand = loadInst->getPointerOperand();

      const auto vaListMemLocationSeq =
          getVaListMemoryLocationSeq(pointerOperand);
      bool isValidMemLocation = !vaListMemLocationSeq.empty();
      if (!isValidMemLocation)
        return false;

      return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                       vaListMemLocationSeq);
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableVaListArgument(const llvm::Value *SrcValue,
                                              const ExtendedValue &Fact) {
  bool isVarArgFact = Fact.isVarArg();
  bool isArgEqual = isArgumentEqual(SrcValue, Fact, isVarArgFact);

  return isVarArgFact && isArgEqual;
}

bool DataFlowUtils::isPatchableArgumentMemcpy(
    const llvm::Value *SrcValue,
    const std::vector<const llvm::Value *> SrcMemLocationSeq,
    const ExtendedValue &Fact) {
  bool isVarArgFact = Fact.isVarArg();
  if (!isVarArgFact)
    return false;

  bool isIndexEqual = Fact.getVarArgIndex() == Fact.getCurrentVarArgIndex();
  if (!isIndexEqual)
    return false;

  bool isSrcMemLocation = !SrcMemLocationSeq.empty();
  if (isSrcMemLocation) {

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                     SrcMemLocationSeq);
  } else if (const auto bitCastInst =
                 llvm::dyn_cast<llvm::BitCastInst>(SrcValue)) {
    const auto pointerOperand = bitCastInst->getOperand(0);

    const auto vaListMemLocationSeq =
        getVaListMemoryLocationSeq(pointerOperand);
    bool isValidMemLocation = !vaListMemLocationSeq.empty();
    if (!isValidMemLocation)
      return false;

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(Fact),
                                     vaListMemLocationSeq);
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
  const auto factMemLocationFrame = getMemoryLocationFrameFromFact(Fact);
  if (!factMemLocationFrame)
    return false;

  if (const auto patchableCallInst =
          llvm::dyn_cast<llvm::CallInst>(factMemLocationFrame)) {

    if (const auto srcValueExtractValueInst =
            llvm::dyn_cast<llvm::ExtractValueInst>(SrcValue)) {
      bool isLinkEqual =
          srcValueExtractValueInst->getAggregateOperand() == patchableCallInst;
      if (isLinkEqual)
        return true;
    } else if (const auto srcValueCallInst =
                   llvm::dyn_cast<llvm::CallInst>(SrcValue)) {
      bool isLinkEqual = srcValueCallInst == patchableCallInst;
      if (isLinkEqual)
        return true;
    }
  }

  return false;
}

const std::vector<const llvm::Value *> DataFlowUtils::patchMemoryLocationFrame(
    const std::vector<const llvm::Value *> PatchableMemLocationSeq,
    const std::vector<const llvm::Value *> PatchMemLocationSeq) {
  if (PatchableMemLocationSeq.empty())
    return EMPTY_SEQ;
  if (PatchMemLocationSeq.empty())
    return EMPTY_SEQ;

  std::vector<const llvm::Value *> patchedMemLocationSeq;
  patchedMemLocationSeq.reserve((PatchableMemLocationSeq.size() - 1) +
                                PatchMemLocationSeq.size());

  patchedMemLocationSeq.insert(patchedMemLocationSeq.end(),
                               PatchMemLocationSeq.begin(),
                               PatchMemLocationSeq.end());
  patchedMemLocationSeq.insert(patchedMemLocationSeq.end(),
                               std::next(PatchableMemLocationSeq.begin()),
                               PatchableMemLocationSeq.end());

  return patchedMemLocationSeq;
}

static long getNumCoercedArgs(const llvm::Value *Value) {
  if (const auto constExpr = llvm::dyn_cast<llvm::ConstantExpr>(Value)) {
    Value = const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  if (llvm::isa<llvm::AllocaInst>(Value) ||
      llvm::isa<llvm::GlobalVariable>(Value)) {
    return -4711;
  }

  if (const auto bitCastInst = llvm::dyn_cast<llvm::BitCastInst>(Value)) {
    long ret = getNumCoercedArgs(bitCastInst->getOperand(0));

    if (ret == -4711) {
      const auto dstType = bitCastInst->getDestTy();
      if (!dstType->isPointerTy())
        return -1;

      const auto elementType = dstType->getPointerElementType();

      if (const auto structType =
              llvm::dyn_cast<llvm::StructType>(elementType)) {
        return static_cast<long>(structType->getNumElements());
      }
      return -1;
    }

    return ret;
  } else if (const auto gepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(Value)) {
    return getNumCoercedArgs(gepInst->getPointerOperand());
  } else if (const auto loadInst = llvm::dyn_cast<llvm::LoadInst>(Value)) {
    return getNumCoercedArgs(loadInst->getPointerOperand());
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
const std::vector<
    std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
               const llvm::Value *>>
DataFlowUtils::getSanitizedArgList(const llvm::CallInst *CallInst,
                                   const llvm::Function *DestFun,
                                   const llvm::Value *ZeroValue) {
  std::vector<
      std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
                 const llvm::Value *>>
      sanitizedArgList;

  for (unsigned i = 0; i < CallInst->getNumArgOperands(); ++i) {
    const auto arg = CallInst->getOperand(i);
    const auto param = getNthFunctionArgument(DestFun, i);

    auto argMemLocationSeq = DataFlowUtils::getMemoryLocationSeqFromMatr(arg);

    long numCoersedArgs = getNumCoercedArgs(arg);
    bool isCoersedArg = numCoersedArgs > 0;

    bool isArrayDecay = DataFlowUtils::isArrayDecay(arg);

    if (isCoersedArg) {
      argMemLocationSeq.pop_back();
      i += numCoersedArgs - 1;
    } else if (isArrayDecay) {
      argMemLocationSeq.pop_back();
    }

    const auto sanitizedParam = param ? param : ZeroValue;

    sanitizedArgList.push_back(
        std::make_tuple(arg, argMemLocationSeq, sanitizedParam));
  }

  return sanitizedArgList;
}

static const std::vector<llvm::BasicBlock *> getPostDominators(
    const llvm::DomTreeNodeBase<llvm::BasicBlock> *PostDomTreeNode,
    const llvm::BasicBlock *StartBasicBlock) {
  const auto currentBasicBlock = PostDomTreeNode->getBlock();
  bool isStartBasicBlock = currentBasicBlock == StartBasicBlock;

  if (isStartBasicBlock)
    return {currentBasicBlock};

  for (const auto postDomTreeChild : PostDomTreeNode->getChildren()) {
    auto childNodes = getPostDominators(postDomTreeChild, StartBasicBlock);
    if (!childNodes.empty()) {
      childNodes.push_back(currentBasicBlock);

      return childNodes;
    }
  }

  return std::vector<llvm::BasicBlock *>();
}

const llvm::BasicBlock *
DataFlowUtils::getEndOfTaintedBlock(const llvm::BasicBlock *StartBasicBlock) {
  const auto terminatorInst = StartBasicBlock->getTerminator();
  const auto function =
      const_cast<llvm::Function *>(StartBasicBlock->getParent());

  bool isBlockStatement = llvm::isa<llvm::BranchInst>(terminatorInst) ||
                          llvm::isa<llvm::SwitchInst>(terminatorInst);
  if (!isBlockStatement)
    return nullptr;

  llvm::PostDominatorTree postDominatorTree;
  postDominatorTree.recalculate(*function);

  const auto postDominators =
      getPostDominators(postDominatorTree.getRootNode(), StartBasicBlock);

  return postDominators.size() > 1 ? postDominators[1] : nullptr;
}

/*
 * We are removing the tainted branch instruction from facts if the
 * instruction's basic block label matches the one of the tainted branch end
 * block. Note that we remove it after the phi node making sure that the phi
 * node is auto added whenever we came from a tainted branch.
 */
bool DataFlowUtils::removeTaintedBlockInst(
    const ExtendedValue &Fact, const llvm::Instruction *CurrentInst) {
  bool isEndOfFunctionTaint = Fact.getEndOfTaintedBlockLabel().empty();
  if (isEndOfFunctionTaint)
    return false;

  bool isPhiNode = llvm::isa<llvm::PHINode>(CurrentInst);
  if (isPhiNode)
    return false;

  const auto currentBB = CurrentInst->getParent();
  const auto currentLabel = currentBB->getName();

  return currentLabel == Fact.getEndOfTaintedBlockLabel();
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
  bool isLoad = llvm::isa<llvm::LoadInst>(CurrentInst);
  if (isLoad)
    return false;

  return llvm::isa<llvm::UnaryInstruction>(CurrentInst) ||
         llvm::isa<llvm::BinaryOperator>(CurrentInst) ||
         llvm::isa<llvm::CmpInst>(CurrentInst) ||
         llvm::isa<llvm::SelectInst>(CurrentInst);
}

bool DataFlowUtils::isAutoIdentity(const llvm::Instruction *CurrentInst,
                                   const ExtendedValue &Fact) {
  bool isVarArgTemplate = Fact.isVarArgTemplate();
  if (isVarArgTemplate) {

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
  if (const auto storeInst = llvm::dyn_cast<llvm::StoreInst>(CurrentInst)) {
    const auto srcMemLocationMatr = storeInst->getValueOperand();
    const auto srcMemLocationFrame =
        getMemoryLocationFrameFromMatr(srcMemLocationMatr);

    bool isArgumentPatch =
        llvm::isa_and_nonnull<llvm::Argument>(srcMemLocationFrame);
    if (isArgumentPatch)
      return false;

    const auto dstMemLocationMatr = storeInst->getPointerOperand();
    const auto dstMemLocationSeq =
        getMemoryLocationSeqFromMatr(dstMemLocationMatr);

    bool isDstMemLocation = !dstMemLocationSeq.empty();
    if (isDstMemLocation) {
      const auto memLocationFrameType = dstMemLocationSeq.front()->getType();

      bool isMemLocationFrameTypeVaList = isVaListType(memLocationFrameType);
      if (isMemLocationFrameTypeVaList)
        return true;
    }
  }

  return false;
}

bool DataFlowUtils::isVarArgParam(const llvm::Value *Param,
                                  const llvm::Value *ZeroValue) {
  return Param == ZeroValue;
}

bool DataFlowUtils::isVaListType(const llvm::Type *Type) {
  const auto typeName = getTypeName(Type);

  return typeName.find("%struct.__va_list_tag") != std::string::npos;
}

bool DataFlowUtils::isReturnValue(const llvm::Instruction *CurrentInst,
                                  const llvm::Instruction *SuccessorInst) {
  bool isSuccessorRetVal = llvm::isa<llvm::ReturnInst>(SuccessorInst);
  if (!isSuccessorRetVal)
    return false;

  if (const auto binaryOpInst =
          llvm::dyn_cast<llvm::BinaryOperator>(CurrentInst)) {
    bool isMagicOpCode = binaryOpInst->getOpcode() == 20;
    if (!isMagicOpCode)
      return false;

    bool isMagicType = getTypeName(binaryOpInst->getType()) == "i4711";
    if (!isMagicType)
      return false;

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
  if (!MemLocationMatr)
    return false;

  if (const auto constExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(MemLocationMatr)) {
    MemLocationMatr =
        const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  bool isMemLocationFrame = isMemoryLocationFrame(MemLocationMatr);
  if (isMemLocationFrame) {
    return false;
  } else if (const auto castInst =
                 llvm::dyn_cast<llvm::CastInst>(MemLocationMatr)) {
    return isArrayDecay(castInst->getOperand(0));
  } else if (const auto gepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(MemLocationMatr)) {
    bool isSrcMemLocationArrayType =
        gepInst->getPointerOperandType()->getPointerElementType()->isArrayTy();
    if (isSrcMemLocationArrayType)
      return true;

    return false;
  } else if (const auto loadInst =
                 llvm::dyn_cast<llvm::LoadInst>(MemLocationMatr)) {
    return false;
  }

  return false;
}

bool DataFlowUtils::isGlobalMemoryLocationSeq(
    const std::vector<const llvm::Value *> MemLocationSeq) {
  if (MemLocationSeq.empty())
    return false;

  return llvm::isa<llvm::GlobalVariable>(MemLocationSeq.front());
}

static void
dumpMemoryLocation(const std::vector<const llvm::Value *> MemLocationSeq) {
#ifdef DEBUG_BUILD
  for (const auto memLocationPart : MemLocationSeq) {
    llvm::outs() << "[ENV_TRACE] ";
    memLocationPart->print(llvm::outs());
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

static const std::set<std::string> readFileFromEnvVar(const char *EnvVar) {
  std::set<std::string> lines;

  const char *filePath = std::getenv(EnvVar);
  if (!filePath) {
    LOG_INFO(EnvVar << " unset");
    return lines;
  } else {
    LOG_INFO(EnvVar << " set to: " << filePath);
  }

  std::ifstream fis(filePath);
  if (fis.fail()) {
    LOG_INFO("Failed to read from: " << filePath);
    return lines;
  }

  std::string line;
  while (std::getline(fis, line)) {
    if (line.empty())
      continue;
    if (line.at(0) == '#')
      continue;

    lines.insert(line);
  }

  return lines;
}

const std::set<std::string> DataFlowUtils::getTaintedFunctions() {
  std::set<std::string> taintedFunctions =
      readFileFromEnvVar("TAINTED_FUNCTIONS_LOCATION");
  if (taintedFunctions.empty())
    taintedFunctions = {"getenv", "secure_getenv"};

  LOG_INFO("Tainted functions:");
  for (const auto &taintedFunction : taintedFunctions) {
    LOG_INFO(taintedFunction);
  }

  return taintedFunctions;
}

const std::set<std::string> DataFlowUtils::getBlacklistedFunctions() {
  std::set<std::string> blacklistedFunctions =
      readFileFromEnvVar("BLACKLISTED_FUNCTIONS_LOCATION");
  if (blacklistedFunctions.empty())
    blacklistedFunctions = {"printf"};

  LOG_INFO("Blacklisted functions:");
  for (const auto &blacklistedFunction : blacklistedFunctions) {
    LOG_INFO(blacklistedFunction);
  }

  return blacklistedFunctions;
}

const std::string
DataFlowUtils::getTraceFilenamePrefix(std::string EntryPoint) {
  time_t time = std::time(nullptr);
  long now = static_cast<long>(time);

  std::stringstream traceFileStream;
  traceFileStream << "static"
                  << "-" << EntryPoint << "-" << now;

  return traceFileStream.str();
}
