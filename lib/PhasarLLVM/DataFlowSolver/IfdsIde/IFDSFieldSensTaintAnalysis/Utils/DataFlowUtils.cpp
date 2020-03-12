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

static const std::string getTypeName(const llvm::Type *type) {
  std::string typeName;
  llvm::raw_string_ostream typeRawOutputStream(typeName);
  type->print(typeRawOutputStream);

  return typeRawOutputStream.str();
}

static bool isMemoryLocationFrame(const llvm::Value *memLocationPart) {
  return llvm::isa<llvm::AllocaInst>(memLocationPart) ||
         llvm::isa<llvm::Argument>(memLocationPart) ||
         llvm::isa<llvm::GlobalVariable>(memLocationPart);
}

static bool isConstantIntEqual(const llvm::ConstantInt *ci1,
                               const llvm::ConstantInt *ci2) {
  // Compare numerical value without type
  // return ci1->getSExtValue() == ci2->getSExtValue();

  // Compare with type
  return ci1 == ci2;
}

static bool isGEPPartEqual(const llvm::GetElementPtrInst *memLocationFactGEP,
                           const llvm::GetElementPtrInst *memLocationInstGEP) {
  bool haveValidGEPParts = memLocationFactGEP->hasAllConstantIndices() &&
                           memLocationInstGEP->hasAllConstantIndices();
  if (!haveValidGEPParts)
    return false;

  bool isNumIndicesEqual = memLocationFactGEP->getNumIndices() ==
                           memLocationInstGEP->getNumIndices();

  if (isNumIndicesEqual) {
    // Compare pointer type
    const auto gepFactPtrType = memLocationFactGEP->getPointerOperandType();
    const auto gepInstPtrType = memLocationInstGEP->getPointerOperandType();
    if (gepFactPtrType != gepInstPtrType)
      return false;

    // Compare indices
    for (unsigned int i = 1; i < memLocationFactGEP->getNumOperands(); i++) {
      const auto *gepFactIndex =
          llvm::cast<llvm::ConstantInt>(memLocationFactGEP->getOperand(i));
      const auto *gepInstIndex =
          llvm::cast<llvm::ConstantInt>(memLocationInstGEP->getOperand(i));

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
    const auto nonDecayedArrayGEP = memLocationFactGEP->getNumIndices() >
                                            memLocationInstGEP->getNumIndices()
                                        ? memLocationFactGEP
                                        : memLocationInstGEP;

    if (const auto nonDecayedArrayGEPPtrIndex =
            llvm::dyn_cast<llvm::ConstantInt>(
                nonDecayedArrayGEP->getOperand(1))) {
      if (!nonDecayedArrayGEPPtrIndex->isZero())
        return false;
    } else {
      return false;
    }

    const auto *gepFactIndex =
        llvm::cast<llvm::ConstantInt>(memLocationFactGEP->getOperand(
            memLocationFactGEP->getNumOperands() - 1));
    const auto *gepInstIndex =
        llvm::cast<llvm::ConstantInt>(memLocationInstGEP->getOperand(
            memLocationInstGEP->getNumOperands() - 1));

    return isConstantIntEqual(gepFactIndex, gepInstIndex);
  }

  return true;
}

static bool isFirstNMemoryLocationPartsEqual(
    std::vector<const llvm::Value *> memLocationSeqFact,
    std::vector<const llvm::Value *> memLocationSeqInst, std::size_t n) {
  assert(n > 0);

  bool seqsHaveAtLeastNParts =
      memLocationSeqFact.size() >= n && memLocationSeqInst.size() >= n;
  if (!seqsHaveAtLeastNParts)
    return false;

  bool haveMemLocationFrames =
      isMemoryLocationFrame(memLocationSeqFact.front()) &&
      isMemoryLocationFrame(memLocationSeqInst.front());
  if (!haveMemLocationFrames)
    return false;

  static_assert(
      true,
      "We have vectors that both start with a memory location"
      "frame.Size may differ but we have at least n instances in each each.");

  bool isSameMemLocationFrame =
      memLocationSeqFact.front() == memLocationSeqInst.front();
  if (!isSameMemLocationFrame)
    return false;

  for (std::size_t i = 1; i < n; ++i) {
    const auto factGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(memLocationSeqFact[i]);
    const auto instGEPPtr =
        llvm::cast<llvm::GetElementPtrInst>(memLocationSeqInst[i]);

    bool isEqual = isGEPPartEqual(factGEPPtr, instGEPPtr);
    if (!isEqual)
      return false;
  }

  return true;
}

static bool isUnionBitCast(const llvm::CastInst *castInst) {
  if (const auto bitCastInst = llvm::dyn_cast<llvm::BitCastInst>(castInst)) {
    const auto typeName = getTypeName(bitCastInst->getSrcTy());

    return typeName.find("union") != std::string::npos;
  }
  return false;
}

static std::vector<const llvm::Value *>
getMemoryLocationSeqFromMatrRec(const llvm::Value *memLocationPart) {
  // Globals
  if (const auto constExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(memLocationPart)) {
    memLocationPart =
        const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  std::vector<const llvm::Value *> memLocationSeq;

  bool isMemLocationFrame = isMemoryLocationFrame(memLocationPart);
  if (isMemLocationFrame) {
    memLocationSeq.push_back(memLocationPart);

    return memLocationSeq;
  }

  if (const auto castInst = llvm::dyn_cast<llvm::CastInst>(memLocationPart)) {
    memLocationSeq = getMemoryLocationSeqFromMatrRec(castInst->getOperand(0));

    bool poisonSeq = isUnionBitCast(castInst);
    if (!poisonSeq)
      return memLocationSeq;

    // FALLTHROUGH
  } else if (const auto loadInst =
                 llvm::dyn_cast<llvm::LoadInst>(memLocationPart)) {
    return getMemoryLocationSeqFromMatrRec(loadInst->getOperand(0));
  } else if (const auto gepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(memLocationPart)) {
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
normalizeGlobalGEPs(const std::vector<const llvm::Value *> memLocationSeq) {
  bool isGlobalMemLocationSeq =
      DataFlowUtils::isGlobalMemoryLocationSeq(memLocationSeq);
  if (!isGlobalMemLocationSeq)
    return memLocationSeq;

  std::vector<const llvm::Value *> normalizedMemLocationSeq;
  normalizedMemLocationSeq.push_back(memLocationSeq.front());

  for (std::size_t i = 1; i < memLocationSeq.size(); ++i) {
    const auto gepInst = llvm::cast<llvm::GetElementPtrInst>(memLocationSeq[i]);

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
normalizeMemoryLocationSeq(std::vector<const llvm::Value *> memLocationSeq) {
  assert(!memLocationSeq.empty());

  // Remove poison pill
  bool isSeqPoisoned = memLocationSeq.back() == POISON_PILL;
  if (isSeqPoisoned)
    memLocationSeq.pop_back();

  if (memLocationSeq.empty())
    return memLocationSeq;

  // Normalize global GEP parts
  memLocationSeq = normalizeGlobalGEPs(memLocationSeq);

  return memLocationSeq;
}

const std::vector<const llvm::Value *>
DataFlowUtils::getMemoryLocationSeqFromMatr(
    const llvm::Value *memLocationMatr) {
  auto memLocationSeq = normalizeMemoryLocationSeq(
      getMemoryLocationSeqFromMatrRec(memLocationMatr));

  assert(memLocationSeq.empty() ||
         isMemoryLocationFrame(memLocationSeq.front()));

  return memLocationSeq;
}

const std::vector<const llvm::Value *>
DataFlowUtils::getMemoryLocationSeqFromFact(
    const ExtendedValue &memLocationFact) {
  return memLocationFact.getMemLocationSeq();
}

const std::vector<const llvm::Value *>
DataFlowUtils::getVaListMemoryLocationSeqFromFact(
    const ExtendedValue &vaListFact) {
  return vaListFact.getVaListMemLocationSeq();
}

static const llvm::Value *
getMemoryLocationFrameFromFact(const ExtendedValue &memLocationFact) {
  const auto memLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(memLocationFact);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

static const llvm::Value *
getVaListMemoryLocationFrameFromFact(const ExtendedValue &vaListFact) {
  const auto memLocationSeq =
      DataFlowUtils::getVaListMemoryLocationSeqFromFact(vaListFact);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

static const llvm::Value *
getMemoryLocationFrameFromMatr(const llvm::Value *memLocationMatr) {
  const auto memLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(memLocationMatr);
  if (memLocationSeq.empty())
    return nullptr;

  return memLocationSeq.front();
}

bool DataFlowUtils::isValueTainted(const llvm::Value *currentInst,
                                   const ExtendedValue &fact) {
  return currentInst == fact.getValue();
}

bool DataFlowUtils::isMemoryLocationTainted(const llvm::Value *memLocationMatr,
                                            const ExtendedValue &fact) {
  auto memLocationInstSeq = getMemoryLocationSeqFromMatr(memLocationMatr);
  if (memLocationInstSeq.empty())
    return false;

  const auto memLocationFactSeq = getMemoryLocationSeqFromFact(fact);
  if (memLocationFactSeq.empty())
    return false;

  bool isArrayDecay = DataFlowUtils::isArrayDecay(memLocationMatr);
  if (isArrayDecay)
    memLocationInstSeq.pop_back();

  return isSubsetMemoryLocationSeq(memLocationInstSeq, memLocationFactSeq);
}

bool DataFlowUtils::isMemoryLocationSeqsEqual(
    const std::vector<const llvm::Value *> memLocationSeq1,
    const std::vector<const llvm::Value *> memLocationSeq2) {
  bool isSizeEqual = memLocationSeq1.size() == memLocationSeq2.size();
  if (!isSizeEqual)
    return false;

  bool isEmptySeq = memLocationSeq1.empty();
  if (isEmptySeq)
    return false;

  std::size_t n = memLocationSeq1.size();
  bool isMemLocationsEqual =
      isFirstNMemoryLocationPartsEqual(memLocationSeq1, memLocationSeq2, n);
  if (!isMemLocationsEqual)
    return false;

  return true;
}

bool DataFlowUtils::isSubsetMemoryLocationSeq(
    const std::vector<const llvm::Value *> memLocationSeqInst,
    const std::vector<const llvm::Value *> memLocationSeqFact) {
  if (memLocationSeqInst.empty())
    return false;
  if (memLocationSeqFact.empty())
    return false;

  std::size_t n = std::min<std::size_t>(memLocationSeqInst.size(),
                                        memLocationSeqFact.size());

  return isFirstNMemoryLocationPartsEqual(memLocationSeqInst,
                                          memLocationSeqFact, n);
}

const std::vector<const llvm::Value *>
DataFlowUtils::getRelocatableMemoryLocationSeq(
    const std::vector<const llvm::Value *> taintedMemLocationSeq,
    const std::vector<const llvm::Value *> srcMemLocationSeq) {
  std::vector<const llvm::Value *> relocatableMemLocationSeq;

  for (std::size_t i = srcMemLocationSeq.size();
       i < taintedMemLocationSeq.size(); ++i) {
    relocatableMemLocationSeq.push_back(taintedMemLocationSeq[i]);
  }

  return relocatableMemLocationSeq;
}

const std::vector<const llvm::Value *> DataFlowUtils::joinMemoryLocationSeqs(
    const std::vector<const llvm::Value *> memLocationSeq1,
    const std::vector<const llvm::Value *> memLocationSeq2) {
  std::vector<const llvm::Value *> joinedMemLocationSeq;
  joinedMemLocationSeq.reserve(memLocationSeq1.size() + memLocationSeq2.size());

  joinedMemLocationSeq.insert(joinedMemLocationSeq.end(),
                              memLocationSeq1.begin(), memLocationSeq1.end());
  joinedMemLocationSeq.insert(joinedMemLocationSeq.end(),
                              memLocationSeq2.begin(), memLocationSeq2.end());

  return joinedMemLocationSeq;
}

const std::vector<const llvm::Value *>
getVaListMemoryLocationSeq(const llvm::Value *value) {
  if (const auto phiNodeInst = llvm::dyn_cast<llvm::PHINode>(value)) {
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

static bool isArgumentEqual(const llvm::Value *srcValue,
                            const ExtendedValue &fact, bool isVarArgFact) {
  const auto factMemLocationFrame =
      isVarArgFact ? getVaListMemoryLocationFrameFromFact(fact)
                   : getMemoryLocationFrameFromFact(fact);
  if (!factMemLocationFrame)
    return false;

  if (const auto patchableArgument =
          llvm::dyn_cast<llvm::Argument>(factMemLocationFrame)) {
    if (patchableArgument->hasByValAttr())
      return false;

    if (const auto srcValueArgument =
            llvm::dyn_cast<llvm::Argument>(srcValue)) {
      bool isLinkEqual = srcValueArgument == patchableArgument;
      if (isLinkEqual)
        return true;
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableArgumentStore(const llvm::Value *srcValue,
                                             const ExtendedValue &fact) {
  bool isVarArgFact = fact.isVarArg();

  bool isArgEqual = isArgumentEqual(srcValue, fact, isVarArgFact);
  if (isArgEqual)
    return true;

  /*
   * Patch of varargs passed through '...'
   */
  if (isVarArgFact) {
    bool isIndexEqual = fact.getVarArgIndex() == fact.getCurrentVarArgIndex();
    if (!isIndexEqual)
      return false;

    if (const auto loadInst = llvm::dyn_cast<llvm::LoadInst>(srcValue)) {
      const auto pointerOperand = loadInst->getPointerOperand();

      const auto vaListMemLocationSeq =
          getVaListMemoryLocationSeq(pointerOperand);
      bool isValidMemLocation = !vaListMemLocationSeq.empty();
      if (!isValidMemLocation)
        return false;

      return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(fact),
                                       vaListMemLocationSeq);
    }
  }

  return false;
}

bool DataFlowUtils::isPatchableVaListArgument(const llvm::Value *srcValue,
                                              const ExtendedValue &fact) {
  bool isVarArgFact = fact.isVarArg();
  bool isArgEqual = isArgumentEqual(srcValue, fact, isVarArgFact);

  return isVarArgFact && isArgEqual;
}

bool DataFlowUtils::isPatchableArgumentMemcpy(
    const llvm::Value *srcValue,
    const std::vector<const llvm::Value *> srcMemLocationSeq,
    const ExtendedValue &fact) {
  bool isVarArgFact = fact.isVarArg();
  if (!isVarArgFact)
    return false;

  bool isIndexEqual = fact.getVarArgIndex() == fact.getCurrentVarArgIndex();
  if (!isIndexEqual)
    return false;

  bool isSrcMemLocation = !srcMemLocationSeq.empty();
  if (isSrcMemLocation) {

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(fact),
                                     srcMemLocationSeq);
  } else if (const auto bitCastInst =
                 llvm::dyn_cast<llvm::BitCastInst>(srcValue)) {
    const auto pointerOperand = bitCastInst->getOperand(0);

    const auto vaListMemLocationSeq =
        getVaListMemoryLocationSeq(pointerOperand);
    bool isValidMemLocation = !vaListMemLocationSeq.empty();
    if (!isValidMemLocation)
      return false;

    return isSubsetMemoryLocationSeq(getVaListMemoryLocationSeqFromFact(fact),
                                     vaListMemLocationSeq);
  }

  return false;
}

bool DataFlowUtils::isPatchableReturnValue(const llvm::Value *srcValue,
                                           const ExtendedValue &fact) {
  /*
   * We could also check against the fact which is also a call inst when we
   * have a return value. However as we are not changing the fact after
   * relocation it would be again taken into account. If we use the patch
   * part it is gone after first patch.
   */
  const auto factMemLocationFrame = getMemoryLocationFrameFromFact(fact);
  if (!factMemLocationFrame)
    return false;

  if (const auto patchableCallInst =
          llvm::dyn_cast<llvm::CallInst>(factMemLocationFrame)) {

    if (const auto srcValueExtractValueInst =
            llvm::dyn_cast<llvm::ExtractValueInst>(srcValue)) {
      bool isLinkEqual =
          srcValueExtractValueInst->getAggregateOperand() == patchableCallInst;
      if (isLinkEqual)
        return true;
    } else if (const auto srcValueCallInst =
                   llvm::dyn_cast<llvm::CallInst>(srcValue)) {
      bool isLinkEqual = srcValueCallInst == patchableCallInst;
      if (isLinkEqual)
        return true;
    }
  }

  return false;
}

const std::vector<const llvm::Value *> DataFlowUtils::patchMemoryLocationFrame(
    const std::vector<const llvm::Value *> patchableMemLocationSeq,
    const std::vector<const llvm::Value *> patchMemLocationSeq) {
  if (patchableMemLocationSeq.empty())
    return EMPTY_SEQ;
  if (patchMemLocationSeq.empty())
    return EMPTY_SEQ;

  std::vector<const llvm::Value *> patchedMemLocationSeq;
  patchedMemLocationSeq.reserve((patchableMemLocationSeq.size() - 1) +
                                patchMemLocationSeq.size());

  patchedMemLocationSeq.insert(patchedMemLocationSeq.end(),
                               patchMemLocationSeq.begin(),
                               patchMemLocationSeq.end());
  patchedMemLocationSeq.insert(patchedMemLocationSeq.end(),
                               std::next(patchableMemLocationSeq.begin()),
                               patchableMemLocationSeq.end());

  return patchedMemLocationSeq;
}

static long getNumCoercedArgs(const llvm::Value *value) {
  if (const auto constExpr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
    value = const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  if (llvm::isa<llvm::AllocaInst>(value) ||
      llvm::isa<llvm::GlobalVariable>(value)) {
    return -4711;
  }

  if (const auto bitCastInst = llvm::dyn_cast<llvm::BitCastInst>(value)) {
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
                 llvm::dyn_cast<llvm::GetElementPtrInst>(value)) {
    return getNumCoercedArgs(gepInst->getPointerOperand());
  } else if (const auto loadInst = llvm::dyn_cast<llvm::LoadInst>(value)) {
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
DataFlowUtils::getSanitizedArgList(const llvm::CallInst *callInst,
                                   const llvm::Function *destFun,
                                   const llvm::Value *zeroValue) {
  std::vector<
      std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
                 const llvm::Value *>>
      sanitizedArgList;

  for (unsigned i = 0; i < callInst->getNumArgOperands(); ++i) {
    const auto arg = callInst->getOperand(i);
    const auto param = getNthFunctionArgument(destFun, i);

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

    const auto sanitizedParam = param ? param : zeroValue;

    sanitizedArgList.push_back(
        std::make_tuple(arg, argMemLocationSeq, sanitizedParam));
  }

  return sanitizedArgList;
}

static const std::vector<llvm::BasicBlock *> getPostDominators(
    const llvm::DomTreeNodeBase<llvm::BasicBlock> *postDomTreeNode,
    const llvm::BasicBlock *startBasicBlock) {
  const auto currentBasicBlock = postDomTreeNode->getBlock();
  bool isStartBasicBlock = currentBasicBlock == startBasicBlock;

  if (isStartBasicBlock)
    return {currentBasicBlock};

  for (const auto postDomTreeChild : postDomTreeNode->getChildren()) {
    auto childNodes = getPostDominators(postDomTreeChild, startBasicBlock);
    if (!childNodes.empty()) {
      childNodes.push_back(currentBasicBlock);

      return childNodes;
    }
  }

  return std::vector<llvm::BasicBlock *>();
}

const llvm::BasicBlock *
DataFlowUtils::getEndOfTaintedBlock(const llvm::BasicBlock *startBasicBlock) {
  const auto terminatorInst = startBasicBlock->getTerminator();
  const auto function =
      const_cast<llvm::Function *>(startBasicBlock->getParent());

  bool isBlockStatement = llvm::isa<llvm::BranchInst>(terminatorInst) ||
                          llvm::isa<llvm::SwitchInst>(terminatorInst);
  if (!isBlockStatement)
    return nullptr;

  llvm::PostDominatorTree postDominatorTree;
  postDominatorTree.recalculate(*function);

  const auto postDominators =
      getPostDominators(postDominatorTree.getRootNode(), startBasicBlock);

  return postDominators.size() > 1 ? postDominators[1] : nullptr;
}

/*
 * We are removing the tainted branch instruction from facts if the
 * instruction's basic block label matches the one of the tainted branch end
 * block. Note that we remove it after the phi node making sure that the phi
 * node is auto added whenever we came from a tainted branch.
 */
bool DataFlowUtils::removeTaintedBlockInst(
    const ExtendedValue &fact, const llvm::Instruction *currentInst) {
  bool isEndOfFunctionTaint = fact.getEndOfTaintedBlockLabel().empty();
  if (isEndOfFunctionTaint)
    return false;

  bool isPhiNode = llvm::isa<llvm::PHINode>(currentInst);
  if (isPhiNode)
    return false;

  const auto currentBB = currentInst->getParent();
  const auto currentLabel = currentBB->getName();

  return currentLabel == fact.getEndOfTaintedBlockLabel();
}

bool DataFlowUtils::isAutoGENInTaintedBlock(
    const llvm::Instruction *currentInst) {
  return !llvm::isa<llvm::StoreInst>(currentInst) &&
         !llvm::isa<llvm::MemTransferInst>(currentInst) &&
         !llvm::isa<llvm::BranchInst>(currentInst) &&
         !llvm::isa<llvm::SwitchInst>(currentInst) &&
         !llvm::isa<llvm::ReturnInst>(currentInst);
}

bool DataFlowUtils::isMemoryLocationFact(const ExtendedValue &ev) {
  return !ev.getMemLocationSeq().empty();
}

bool DataFlowUtils::isKillAfterStoreFact(const ExtendedValue &ev) {
  return !isMemoryLocationFact(ev) && !llvm::isa<llvm::CallInst>(ev.getValue());
}

bool DataFlowUtils::isCheckOperandsInst(const llvm::Instruction *currentInst) {
  bool isLoad = llvm::isa<llvm::LoadInst>(currentInst);
  if (isLoad)
    return false;

  return llvm::isa<llvm::UnaryInstruction>(currentInst) ||
         llvm::isa<llvm::BinaryOperator>(currentInst) ||
         llvm::isa<llvm::CmpInst>(currentInst) ||
         llvm::isa<llvm::SelectInst>(currentInst);
}

bool DataFlowUtils::isAutoIdentity(const llvm::Instruction *currentInst,
                                   const ExtendedValue &fact) {
  bool isVarArgTemplate = fact.isVarArgTemplate();
  if (isVarArgTemplate) {

    return !llvm::isa<llvm::VAStartInst>(currentInst);
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
  if (const auto storeInst = llvm::dyn_cast<llvm::StoreInst>(currentInst)) {
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

bool DataFlowUtils::isVarArgParam(const llvm::Value *param,
                                  const llvm::Value *zeroValue) {
  return param == zeroValue;
}

bool DataFlowUtils::isVaListType(const llvm::Type *type) {
  const auto typeName = getTypeName(type);

  return typeName.find("%struct.__va_list_tag") != std::string::npos;
}

bool DataFlowUtils::isReturnValue(const llvm::Instruction *currentInst,
                                  const llvm::Instruction *successorInst) {
  bool isSuccessorRetVal = llvm::isa<llvm::ReturnInst>(successorInst);
  if (!isSuccessorRetVal)
    return false;

  if (const auto binaryOpInst =
          llvm::dyn_cast<llvm::BinaryOperator>(currentInst)) {
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
bool DataFlowUtils::isArrayDecay(const llvm::Value *memLocationMatr) {
  if (!memLocationMatr)
    return false;

  if (const auto constExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(memLocationMatr)) {
    memLocationMatr =
        const_cast<llvm::ConstantExpr *>(constExpr)->getAsInstruction();
  }

  bool isMemLocationFrame = isMemoryLocationFrame(memLocationMatr);
  if (isMemLocationFrame) {
    return false;
  } else if (const auto castInst =
                 llvm::dyn_cast<llvm::CastInst>(memLocationMatr)) {
    return isArrayDecay(castInst->getOperand(0));
  } else if (const auto gepInst =
                 llvm::dyn_cast<llvm::GetElementPtrInst>(memLocationMatr)) {
    bool isSrcMemLocationArrayType =
        gepInst->getPointerOperandType()->getPointerElementType()->isArrayTy();
    if (isSrcMemLocationArrayType)
      return true;

    return false;
  } else if (const auto loadInst =
                 llvm::dyn_cast<llvm::LoadInst>(memLocationMatr)) {
    return false;
  }

  return false;
}

bool DataFlowUtils::isGlobalMemoryLocationSeq(
    const std::vector<const llvm::Value *> memLocationSeq) {
  if (memLocationSeq.empty())
    return false;

  return llvm::isa<llvm::GlobalVariable>(memLocationSeq.front());
}

static void
dumpMemoryLocation(const std::vector<const llvm::Value *> memLocationSeq) {
#ifdef DEBUG_BUILD
  for (const auto memLocationPart : memLocationSeq) {
    llvm::outs() << "[ENV_TRACE] ";
    memLocationPart->print(llvm::outs());
    llvm::outs() << "\n";
    llvm::outs().flush();
  }
#endif
}

void DataFlowUtils::dumpFact(const ExtendedValue &ev) {
  if (!ev.getMemLocationSeq().empty()) {
    LOG_DEBUG("memLocationSeq:");
    dumpMemoryLocation(ev.getMemLocationSeq());
  }

  if (!ev.getEndOfTaintedBlockLabel().empty()) {
    LOG_DEBUG("endOfTaintedBlockLabel: " << ev.getEndOfTaintedBlockLabel());
  }

  if (ev.isVarArg()) {
    if (!ev.isVarArgTemplate()) {
      LOG_DEBUG("vaListMemLocationSeq:");
      dumpMemoryLocation(getVaListMemoryLocationSeqFromFact(ev));
    }
    LOG_DEBUG("varArgIndex: " << ev.getVarArgIndex());
    LOG_DEBUG("currentVarArgIndex: " << ev.getCurrentVarArgIndex());
  }
}

static const std::set<std::string> readFileFromEnvVar(const char *envVar) {
  std::set<std::string> lines;

  const char *filePath = std::getenv(envVar);
  if (!filePath) {
    LOG_INFO(envVar << " unset");
    return lines;
  } else {
    LOG_INFO(envVar << " set to: " << filePath);
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
DataFlowUtils::getTraceFilenamePrefix(std::string entryPoint) {
  time_t time = std::time(nullptr);
  long now = static_cast<long>(time);

  std::stringstream traceFileStream;
  traceFileStream << "static"
                  << "-" << entryPoint << "-" << now;

  return traceFileStream.str();
}
