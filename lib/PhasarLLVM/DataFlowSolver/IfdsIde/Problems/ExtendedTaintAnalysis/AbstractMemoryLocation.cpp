/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace psr::detail {

AbstractMemoryLoactionStorage::AbstractMemoryLoactionStorage(
    const llvm::Value *Baseptr, uint32_t Lifetime,
    const llvm::ArrayRef<ptrdiff_t> &Offsets) noexcept
    : Baseptr(Baseptr), Lifetime(Lifetime),
      NumOffsets(uint32_t(Offsets.size())) {
  assert(Baseptr && "The baseptr must not be null!");
  memcpy(this->Offsets, Offsets.begin(), Offsets.size() * sizeof(ptrdiff_t));
}

AbstractMemoryLoactionStorage::AbstractMemoryLoactionStorage(
    const llvm::Value *Baseptr, uint32_t Lifetime) noexcept
    : Baseptr(Baseptr), Lifetime(Lifetime), NumOffsets(0) {
  assert(Baseptr && "The baseptr must not be null!");
}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl()
    : AbstractMemoryLoactionStorage(LLVMZeroValue::getInstance(), 0) {}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, unsigned Lifetime) noexcept
    : AbstractMemoryLoactionStorage(Baseptr, Lifetime) {}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, llvm::SmallVectorImpl<ptrdiff_t> &&Offsets,
    unsigned Lifetime) noexcept
    : AbstractMemoryLoactionStorage(Baseptr, Lifetime, Offsets) {}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *Baseptr, llvm::ArrayRef<ptrdiff_t> Offsets,
    unsigned Lifetime) noexcept
    : AbstractMemoryLoactionStorage(Baseptr, Lifetime, Offsets) {}

bool AbstractMemoryLocationImpl::isZero() const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Baseptr);
}

llvm::ArrayRef<ptrdiff_t> AbstractMemoryLocationImpl::offsets() const {
  return llvm::makeArrayRef(Offsets, NumOffsets);
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
    return offsets().slice(std::max(size_t(1), TV.offsets().size()) - 1);
  }
  return TV.offsets().slice(std::max(1U, NumOffsets) - 1);
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
    PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "MustAlias(" << llvmIRToShortString(base()) << ", "
                << llvmIRToShortString(TV.base()) << ") = " << std::boolalpha
                << (PT.alias(base(), TV.base()) == AliasResult::MustAlias));

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
    PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const {
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

std::ostream &operator<<(std::ostream &OS, const AbstractMemoryLocation &TV) {
  llvm::raw_os_ostream ROS(OS);
  ROS << TV;
  return OS;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const AbstractMemoryLocation &TV) {
  // -> Think about better representation
  OS << "(";
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(TV->base())) {
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

} // namespace psr

namespace llvm {
llvm::hash_code hash_value(const psr::AbstractMemoryLocation &Val) {
  return hash_combine(
      Val->base(), Val->lifetime() == 0,
      hash_combine_range(Val->offsets().begin(), Val->offsets().end()));
}
} // namespace llvm