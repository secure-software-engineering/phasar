/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_os_ostream.h"

#include <cstddef>

namespace psr::detail {

AbstractMemoryLocationStorage::AbstractMemoryLocationStorage(
    const llvm::Value *Baseptr, uint32_t Lifetime, uint32_t NumOffsets) noexcept
    : Baseptr(Baseptr), Lifetime(Lifetime), NumOffsets(NumOffsets) {
  assert(Baseptr && "The baseptr must not be null!");
}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl()
    : AbstractMemoryLocationStorage(LLVMZeroValue::getInstance(), 0) {}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, unsigned Lifetime) noexcept
    : AbstractMemoryLocationStorage(Baseptr, Lifetime) {}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, llvm::SmallVectorImpl<ptrdiff_t> &&Offsets,
    unsigned Lifetime) noexcept
    : AbstractMemoryLocationImpl(Baseptr, llvm::makeArrayRef(Offsets),
                                 Lifetime) {}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, llvm::ArrayRef<ptrdiff_t> Offsets,
    unsigned Lifetime) noexcept
    : AbstractMemoryLocationStorage(Baseptr, Lifetime, Offsets.size()) {
  memcpy(this->getTrailingObjects<ptrdiff_t>(), Offsets.data(),
         Offsets.size() * sizeof(ptrdiff_t));
}

bool AbstractMemoryLocationImpl::isZero() const {
  return LLVMZeroValue::isLLVMZeroValue(Baseptr);
}

llvm::ArrayRef<ptrdiff_t> AbstractMemoryLocationImpl::offsets() const {
  return llvm::makeArrayRef(this->getTrailingObjects<ptrdiff_t>(), NumOffsets);
}

auto AbstractMemoryLocationImpl::computeOffset(
    const llvm::DataLayout &DL, const llvm::GetElementPtrInst *Gep)
    -> std::optional<ptrdiff_t> {
  // TODO: Use results from IDELinearConstantAnalysis here (LLVM 12 has an
  // overload of accumulateConstantOffset that takes an external analysis
  // "https://llvm.org/doxygen/classllvm_1_1GEPOperator.html#a5c00e7e76ef5e98c6ffec8d31f63970a")

  llvm::APInt Ret(sizeof(ptrdiff_t) * 8, 0);
  if (!Gep->accumulateConstantOffset(DL, Ret)) {
    return std::nullopt;
  }

  return Ret.getSExtValue();
}

[[nodiscard]] auto AbstractMemoryLocationImpl::operator-(
    const AbstractMemoryLocationImpl &TV) const -> llvm::ArrayRef<ptrdiff_t> {
  if (NumOffsets > TV.offsets().size()) {
    return offsets().drop_front(std::max(size_t(1), TV.offsets().size()) - 1);
  }
  return TV.offsets().drop_front(std::max(1U, NumOffsets) - 1);
}

bool AbstractMemoryLocationImpl::equivalentOffsets(
    const AbstractMemoryLocationImpl &TV) const {
  size_t MinSize = std::min(offsets().size(), TV.offsets().size());
  return offsets().slice(0, MinSize) == TV.offsets().slice(0, MinSize);
}

bool AbstractMemoryLocationImpl::equivalent(
    const AbstractMemoryLocationImpl &TV) const {
  if (base() != TV.base()) {
    return false;
  }

  return equivalentOffsets(TV);
}

bool AbstractMemoryLocationImpl::equivalentExceptPointerArithmetics(
    const AbstractMemoryLocationImpl &TV, unsigned PALevel) const {
  if (base() != TV.base()) {
    return false;
  }
  size_t MinSize = std::min(offsets().size(), TV.offsets().size());
  if (MinSize <= PALevel) {
    return true;
  }
  return offsets().slice(0, MinSize - PALevel) ==
         TV.offsets().slice(0, MinSize - PALevel);
}

bool AbstractMemoryLocationImpl::mustAlias(
    const AbstractMemoryLocationImpl &TV,
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *> PT) const {
  PHASAR_LOG_LEVEL(DEBUG, "MustAlias(" << llvmIRToShortString(base()) << ", "
                                       << llvmIRToShortString(TV.base())
                                       << ") = "
                                       << (PT.alias(base(), TV.base()) ==
                                           AliasResult::MustAlias));

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto getFunctionOrNull = [](const llvm::Value *V) -> const llvm::Function * {
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
      return Inst->getFunction();
    }
    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
      return Arg->getParent();
    }

    return nullptr;
  };

  if (base() != TV.base() &&
      (getFunctionOrNull(base()) != getFunctionOrNull(TV.base()) ||
       PT.alias(base(), TV.base()) != AliasResult::MustAlias)) {
    return false;
  }

  return equivalentOffsets(TV);
}

bool AbstractMemoryLocationImpl::isProperPrefixOf(
    const AbstractMemoryLocationImpl &Larger) const {
  if (base() != Larger.base()) {
    return false;
  }
  if (offsets().size() >= Larger.offsets().size()) {
    return false;
  }

  return offsets() == Larger.offsets().slice(0, offsets().size());
}

bool AbstractMemoryLocationImpl::isProperPrefixOf(
    const AbstractMemoryLocationImpl &Larger,
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *> PT) const {
  if (base() != Larger.base() &&
      PT.alias(base(), Larger.base()) != AliasResult::MustAlias) {
    return false;
  }
  if (offsets().size() >= Larger.offsets().size()) {
    return false;
  }

  return offsets() == Larger.offsets().slice(0, offsets().size());
}

void AbstractMemoryLocationImpl::MakeProfile(llvm::FoldingSetNodeID &ID,
                                             const llvm::Value *V,
                                             llvm::ArrayRef<ptrdiff_t> Offs,
                                             unsigned Lifetime) {
  assert(!Lifetime || !Offs.empty());
  ID.AddPointer(V);
  for (auto Off : Offs) {
    ID.AddInteger(Off);
  }

  ID.AddBoolean(Lifetime != 0);
}

void AbstractMemoryLocationImpl::Profile(llvm::FoldingSetNodeID &ID) const {
  MakeProfile(ID, Baseptr, offsets(), Lifetime);
}

} // namespace psr::detail

namespace psr {
AbstractMemoryLocation::AbstractMemoryLocation(
    const detail::AbstractMemoryLocationImpl *Impl) noexcept
    : PImpl(Impl) {
  assert(Impl);
}

std::ostream &operator<<(std::ostream &OS, AbstractMemoryLocation TV) {
  llvm::raw_os_ostream ROS(OS);
  ROS << TV;
  return OS;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              AbstractMemoryLocation TV) {
  // -> Think about better representation
  OS << "(";
  if (LLVMZeroValue::isLLVMZeroValue(TV->base())) {
    OS << "<ZERO>";
  } else {
    OS << llvmIRToShortString(TV->base());
  }
  OS << "; Offsets=" << PrettyPrinter{TV->offsets()};

  return OS << " #" << TV->lifetime() << ")";
}

std::string DToString(const AbstractMemoryLocation &AML) {
  std::string Ret;
  llvm::raw_string_ostream OS(Ret);
  OS << AML;
  return OS.str();
}

llvm::hash_code hash_value(psr::AbstractMemoryLocation Val) {
  return hash_combine(
      Val->base(), Val->lifetime() == 0,
      llvm::hash_combine_range(Val->offsets().begin(), Val->offsets().end()));
}
} // namespace psr
