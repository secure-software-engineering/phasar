/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace psr::detail {

AbstractMemoryLoactionStorage::AbstractMemoryLoactionStorage(
    const llvm::Value *baseptr, size_t lifetime,
    const llvm::ArrayRef<ptrdiff_t> &offsets)
    : baseptr_(baseptr), lifetime_(lifetime), numOffsets_(offsets.size()) {
  // std::copy(offsets.begin(), offsets.end(), offsets_);
  assert(baseptr && "The baseptr must not be null!");
  memcpy(offsets_, offsets.begin(), offsets.size() * sizeof(ptrdiff_t));
}

AbstractMemoryLoactionStorage::AbstractMemoryLoactionStorage(
    const llvm::Value *baseptr, size_t lifetime)
    : baseptr_(baseptr), lifetime_(lifetime), numOffsets_(0) {
  assert(baseptr && "The baseptr must not be null!");
}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl()
    : AbstractMemoryLoactionStorage(LLVMZeroValue::getInstance(), 0) {}

AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *baseptr, unsigned lifetime)
    : /* baseptr_(baseptr), lifetime_(lifetime) */
      AbstractMemoryLoactionStorage(baseptr, lifetime) {
  // if (lifetime > 3) {
  //   std::cerr << "Invalid lifetime: " << lifetime
  //             << " on access path with length 0" << std::endl;
  //   abort();
  // }
}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *baseptr, llvm::SmallVectorImpl<offset_t> &&offsets,
    unsigned lifetime)
    : /*baseptr_(baseptr), offsets_(std::move(offsets)), lifetime_(lifetime) */
      AbstractMemoryLoactionStorage(baseptr, lifetime, offsets) {
  // if (offsets.size() + lifetime > 3) {
  //   std::cerr << "Too long field-access-path: " << offsets.size()
  //             << " with lifetime " << lifetime << std::endl;
  //   abort();
  // }
}
AbstractMemoryLocationImpl::AbstractMemoryLocationImpl(
    const llvm::Value *baseptr, llvm::ArrayRef<offset_t> offsets,
    unsigned lifetime)
    : /*baseptr_(baseptr), offsets_(offsets.begin(), offsets.end()),
      lifetime_(lifetime)*/
      AbstractMemoryLoactionStorage(baseptr, lifetime, offsets) {
  // if (offsets.size() + lifetime > 3) {
  //   std::cerr << "Too long field-access-path: " << offsets.size()
  //             << " with lifetime " << lifetime_ << std::endl;
  //   abort();
  // }
}

bool AbstractMemoryLocationImpl::isZero() const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(baseptr_);
}

const llvm::Value *AbstractMemoryLocationImpl::base() const { return baseptr_; }

auto AbstractMemoryLocationImpl::offsets() const -> llvm::ArrayRef<offset_t> {
  return llvm::ArrayRef<offset_t>(offsets_, numOffsets_);
}
uint64_t AbstractMemoryLocationImpl::lifetime() const { return lifetime_; }
auto AbstractMemoryLocationImpl::computeOffset(
    const llvm::DataLayout &DL, const llvm::GetElementPtrInst *Gep)
    -> std::optional<offset_t> {
  // TODO: Use results from IDELinearConstantAnalysis here (LLVM 12 has an
  // overload of accumulateConstantOffset that takes an external analysis
  // "https://llvm.org/doxygen/classllvm_1_1GEPOperator.html#a5c00e7e76ef5e98c6ffec8d31f63970a")

  llvm::APInt ret(sizeof(offset_t) * 8, 0);
  if (!Gep->accumulateConstantOffset(DL, ret)) {
    return std::nullopt;
  }

  return ret.getSExtValue();
}

AbstractMemoryLocationImpl AbstractMemoryLocationImpl::limit() const {
  llvm::SmallVector<offset_t, 5> offs(offsets().begin(), offsets().end());
  offs.pop_back();
  return AbstractMemoryLocationImpl(base(), std::move(offs), 0);
}

[[nodiscard]] auto AbstractMemoryLocationImpl::operator-(
    const AbstractMemoryLocationImpl &TV) const -> llvm::ArrayRef<offset_t> {
  if (numOffsets_ > TV.offsets().size())
    return offsets().slice(std::max(size_t(1), TV.offsets().size()) - 1);
  else
    return TV.offsets().slice(std::max(size_t(1), numOffsets_) - 1);
}

bool AbstractMemoryLocationImpl::equivalentOffsets(
    const AbstractMemoryLocationImpl &TV) const {
  size_t minSize = std::min(offsets().size(), TV.offsets().size());
  return offsets().slice(0, minSize) == TV.offsets().slice(0, minSize);
}

bool AbstractMemoryLocationImpl::equivalent(
    const AbstractMemoryLocationImpl &TV) const {
  if (base() != TV.base())
    return false;

  return equivalentOffsets(TV);
}

bool AbstractMemoryLocationImpl::equivalentExceptPointerArithmetics(
    const AbstractMemoryLocationImpl &TV) const {
  if (base() != TV.base())
    return false;
  size_t minSize = std::min(offsets().size(), TV.offsets().size());
  if (minSize == 1)
    return true;
  return offsets().slice(0, minSize - 1) == TV.offsets().slice(0, minSize - 1);
}

bool AbstractMemoryLocationImpl::mustAlias(
    const AbstractMemoryLocationImpl &TV,
    PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "MustAlias(" << llvmIRToShortString(base()) << ", "
                << llvmIRToShortString(TV.base()) << ") = " << std::boolalpha
                << (PT.alias(base(), TV.base()) == AliasResult::MustAlias));

  auto getFunctionOrNull = [](const llvm::Value *V) -> const llvm::Function * {
    if (auto Inst = llvm::dyn_cast<llvm::Instruction>(V))
      return Inst->getFunction();
    if (auto Arg = llvm::dyn_cast<llvm::Argument>(V))
      return Arg->getParent();

    return nullptr;
  };

  if (base() != TV.base() &&
      (getFunctionOrNull(base()) != getFunctionOrNull(TV.base()) ||
       PT.alias(base(), TV.base()) != AliasResult::MustAlias))
    return false;

  return equivalentOffsets(TV);
}

bool AbstractMemoryLocationImpl::isProperPrefixOf(
    const AbstractMemoryLocationImpl &Larger) const {
  if (base() != Larger.base())
    return false;
  if (offsets().size() >= Larger.offsets().size())
    return false;

  return offsets() == Larger.offsets().slice(0, offsets().size());
}

bool AbstractMemoryLocationImpl::isProperPrefixOf(
    const AbstractMemoryLocationImpl &Larger,
    PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const {
  if (base() != Larger.base() &&
      PT.alias(base(), Larger.base()) != AliasResult::MustAlias)
    return false;
  if (offsets().size() >= Larger.offsets().size())
    return false;

  return offsets() == Larger.offsets().slice(0, offsets().size());
}

// AbstractMemoryLocationImpl
// AbstractMemoryLocationImpl::Create(const llvm::Value *V,
//                                    const llvm::DataLayout &DL, unsigned
//                                    BOUND) {
//   if (llvm::isa<llvm::GlobalValue>(V) || llvm::isa<llvm::AllocaInst>(V) ||
//       llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::CallBase>(V)) {
//     // Globals, argument, function calls and allocas define themselves
//     return AbstractMemoryLocationImpl(V, BOUND);
//   }
//   llvm::SmallVector<uint64_t, 2> offs = {0};
//   unsigned ver = 0;
//   // recursive lambda via Y-combinator
//   const auto createRec =
//       [&DL, BOUND](auto &_createRec, llvm::SmallVectorImpl<uint64_t> &offs,
//                    unsigned &ver, const llvm::Value *V) -> const llvm::Value
//                    * {
//     if (auto load = llvm::dyn_cast<llvm::LoadInst>(V)) {
//       offs.push_back(0);
//       ++ver;
//       return _createRec(_createRec, offs, ver, load->getPointerOperand());
//     }
//     if (auto cast = llvm::dyn_cast<llvm::CastInst>(V)) {
//       return _createRec(_createRec, offs, ver, cast->getOperand(0));
//     }
//     if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(V)) {
//       auto gepOffs = computeOffset(DL, gep);
//       if (gepOffs.has_value()) {
//         offs.back() += *gepOffs;
//         ++ver;
//       } else {
//         // everything until now is irrelevant (constructing the offs-vector
//         in
//         // reverse order)
//         offs.clear();
//         offs.push_back(0);
//         ver = BOUND;
//       }
//       return _createRec(_createRec, offs, ver, gep->getPointerOperand());
//     }
//     // TODO aggregate instructions, e.g. insertvalue, extractvalue, ...

//     return V;
//   };
//   auto baseptr = createRec(createRec, offs, ver, V);
//   std::reverse(offs.begin(), offs.end());

//   return AbstractMemoryLocationImpl(baseptr, std::move(offs),
//                                     BOUND - std::min(ver, BOUND));
// }

void AbstractMemoryLocationImpl::MakeProfile(llvm::FoldingSetNodeID &ID,
                                             const llvm::Value *V,
                                             llvm::ArrayRef<offset_t> offs,
                                             unsigned lifetime) {
  assert(!lifetime || !offs.empty());
  ID.AddPointer(V);
  for (auto off : offs)
    ID.AddInteger(off);

  ID.AddBoolean(lifetime != 0);
}

void AbstractMemoryLocationImpl::Profile(llvm::FoldingSetNodeID &ID) const {
  MakeProfile(ID, baseptr_, offsets(), lifetime_);
}

} // namespace psr::detail

namespace psr {
AbstractMemoryLocation::AbstractMemoryLocation(
    const detail::AbstractMemoryLocationImpl *Impl)
    : pImpl(Impl) {
  assert(Impl);
}

std::ostream &operator<<(std::ostream &os, const AbstractMemoryLocation &TV) {
  llvm::raw_os_ostream ros(os);
  ros << TV;
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const AbstractMemoryLocation &TV) {
  // TODO: better representation

  os << "(";
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(TV->base())) {
    os << "<ZERO>";
  } else {
    os << llvmIRToShortString(TV->base());
  }
  os << "; Offsets=" << SequencePrinter(TV->offsets());

  return os << " #" << TV->lifetime() << ")";
}

std::string DToString(const AbstractMemoryLocation &AML) {
  std::string ret;
  llvm::raw_string_ostream os(ret);
  os << AML;
  return os.str();
}

} // namespace psr

namespace llvm {
llvm::hash_code hash_value(const psr::AbstractMemoryLocation &Val) {
  return hash_combine(
      Val->base(), Val->lifetime() == 0,
      hash_combine_range(Val->offsets().begin(), Val->offsets().end()));
}
} // namespace llvm