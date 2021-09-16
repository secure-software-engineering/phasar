/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <exception>
#include <limits>

#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"
#include "phasar/Utils/Logger.h"

namespace psr::detail {

/// We intentionally don't initialize the Data member as it will be a dynamic
/// array at runtime
// NOLINTNEXTLINE (cppcoreguidelines-pro-type-member-init)
AbstractMemoryLocationFactoryBase::Allocator::Block::Block(Block *Next)
    : Next(Next) {}

auto AbstractMemoryLocationFactoryBase::Allocator::Block::create(
    Block *Next, size_t NumPointerEntries) -> Block * {
  // Allocate one more pointer to store the next-block ptr

  if (NumPointerEntries >
      std::numeric_limits<size_t>::max() / sizeof(size_t) - 1) [[unlikely]] {

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), CRITICAL)
                  << "Cannot allocate " << NumPointerEntries
                  << " pointer entries");

    std::terminate();
  }

  static_assert(
      alignof(AbstractMemoryLocationImpl) == alignof(size_t),
      "The alignment of the AbstractMemoryLocationImpl allocation cannot be "
      "guaranteed as it differs from the alignment of size_t");

  auto *Ret = reinterpret_cast<Block *>(new size_t[1 + NumPointerEntries]);

  new (Ret) Block(Next);
  return Ret;
}

void AbstractMemoryLocationFactoryBase::Allocator::Block::destroy(Block *Blck) {
  delete[] reinterpret_cast<size_t *>(Blck);
}

AbstractMemoryLocationFactoryBase::Allocator::Allocator(
    size_t InitialCapacity) {
  if (InitialCapacity <= ExpectedNumAmLsPerBlock) {
    return;
  }

  const auto NumPointersPerInitialBlock =
      (MinNumPointersPerAML + 3) * InitialCapacity;
  Root = Block::create(nullptr, NumPointersPerInitialBlock);
  Pos = Root->Data;
  End = Pos + NumPointersPerInitialBlock;
}

AbstractMemoryLocationFactoryBase::Allocator::~Allocator() {
  auto *Blck = Root;
  while (Blck) {
    auto *Nxt = Blck->Next;
    Block::destroy(Blck);
    Blck = Nxt;
  }
  Root = nullptr;
  Pos = nullptr;
  End = nullptr;
}

AbstractMemoryLocationFactoryBase::Allocator::Allocator(
    Allocator &&Other) noexcept
    : Root(Other.Root), Pos(Other.Pos), End(Other.End) {
  Other.Root = nullptr;
  Other.Pos = nullptr;
  Other.End = nullptr;
}

auto AbstractMemoryLocationFactoryBase::Allocator::operator=(
    Allocator &&Other) noexcept -> Allocator & {
  this->Allocator::~Allocator();
  new (this) Allocator(std::move(Other));
  return *this;
}

AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::Allocator::create(
    const llvm::Value *Baseptr, size_t Lifetime,
    llvm::ArrayRef<ptrdiff_t> Offsets) {

  // All fields inside AML have pointer size, so there is no padding at all
  auto NumPointersRequired = MinNumPointersPerAML + Offsets.size();
  auto *Rt = Root;
  auto *Curr = Pos;

  if (End - Curr < ptrdiff_t(NumPointersRequired)) {
    Root = Rt = Block::create(Rt, NumPointersPerBlock);
    Pos = Curr = Rt->Data;
    End = Curr + NumPointersPerBlock;
  }

  auto *Ret = reinterpret_cast<AbstractMemoryLocationImpl *>(Curr);

  Pos += NumPointersRequired;

  new (Ret) AbstractMemoryLocationImpl(Baseptr, Offsets, Lifetime);

  return Ret;
}

AbstractMemoryLocationFactoryBase::AbstractMemoryLocationFactoryBase(
    size_t InitialCapacity)
    : Owner(InitialCapacity), DL(nullptr) {

  Pool.reserve(InitialCapacity);
  Cache.reserve(InitialCapacity);
}

AbstractMemoryLocationFactoryBase::AbstractMemoryLocationFactoryBase(
    const llvm::DataLayout *DL, size_t InitialCapacity)
    : Owner(InitialCapacity), DL(DL) {
  assert(DL);

  Pool.reserve(InitialCapacity);
  Cache.reserve(InitialCapacity);
}

void AbstractMemoryLocationFactoryBase::setDataLayout(
    const llvm::DataLayout &DL) {
  assert(!this->DL);
  this->DL = &DL;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::getOrCreateImpl(
    const llvm::Value *V, llvm::SmallVectorImpl<ptrdiff_t> &&Offs,
    unsigned BOUND) {
  llvm::FoldingSetNodeID ID;
  detail::AbstractMemoryLocationImpl::MakeProfile(ID, V, Offs, BOUND);
  void *Pos;
  auto *Mem = Pool.FindNodeOrInsertPos(ID, Pos);
  if (!Mem) {
    Mem = Owner.create(V, BOUND, Offs);
    Pool.InsertNode(Mem, Pos);
  }
  return Mem;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::getOrCreateImpl(const llvm::Value *V,
                                                   unsigned BOUND) {

  llvm::SmallVector<ptrdiff_t, 1> Offs = {0};
  const auto *Ret =
      getOrCreateImpl(V, std::move(Offs), BOUND == 0 ? 0 : BOUND - 1);

  return Ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::CreateImpl(const llvm::Value *V,
                                              unsigned BOUND) {
  assert(DL);
  if (auto It = Cache.find(V); It != Cache.end()) {
    return It->second;
  }

  // Note: llvm::Constant includes llvm::GlobalValue
  if (llvm::isa<llvm::Constant>(V) || llvm::isa<llvm::AllocaInst>(V) ||
      llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::CallBase>(V)) {
    // Globals, argument, function calls and allocas define themselves

    const auto *Mem = getOrCreateImpl(V, BOUND);

    Cache[V] = Mem;

    return Mem;
  }

  llvm::SmallVector<ptrdiff_t, 8> Offs = {0};
  unsigned Ver = 1;

  const auto *Baseptr = V;

  while (true) {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Baseptr)) {
      Offs.push_back(0);
      ++Ver;
      Baseptr = Load->getPointerOperand();
    } else if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Baseptr)) {
      Baseptr = Cast->getOperand(0);
    } else if (const auto *Gep =
                   llvm::dyn_cast<llvm::GetElementPtrInst>(Baseptr)) {

      auto GepOffs =
          detail::AbstractMemoryLocationImpl::computeOffset(*DL, Gep);
      if (GepOffs.has_value()) {
        Offs.back() += *GepOffs;
      } else {
        // everything until now is irrelevant (constructing the offs-vector in
        // reverse order)
        Offs.clear();
        Offs.push_back(0);
        Ver = BOUND;
      }
      Baseptr = Gep->getPointerOperand();
    } else {
      // TODO aggregate instructions, e.g. insertvalue, extractvalue, ...
      break;
    }
  }

  std::reverse(Offs.begin(), Offs.end());
  auto Lifetime = BOUND - std::min(Ver, BOUND);

  // assert(ver >= offs.size());

  bool IsOverApproximating = false;

  if (Offs.size() > BOUND) {
    assert(Lifetime == 0);

    IsOverApproximating = true;
    Offs.resize(BOUND);
  }

  const auto *Mem = getOrCreateImpl(Baseptr, std::move(Offs), Lifetime);

#ifdef XTAINT_DIAGNOSTICS
  if (IsOverApproximating)
    overApproximatedAMLs.insert(mem);
#endif

  Cache[V] = Mem;
  return Mem;
}

[[nodiscard]] const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::GetOrCreateZeroImpl() const {
  // Can allocate without Allocator, because the number of offsets is zero
  static detail::AbstractMemoryLocationImpl Zero = nullptr;
  return &Zero;
}

const AbstractMemoryLocationImpl *AbstractMemoryLocationFactoryBase::limitImpl(
    const AbstractMemoryLocationImpl *AML) {
  const auto *Beg = AML->offsets().begin();
  const auto *End = AML->offsets().end();
  llvm::SmallVector<ptrdiff_t, 8> Offs(Beg, Beg == End ? End : End - 1);
  const auto *Ret = getOrCreateImpl(AML->base(), std::move(Offs), 0);

#ifdef XTAINT_DIAGNOSTICS
  overApproximatedAMLs.insert(ret);
#endif

  return Ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withIndirectionOfImpl(
    const AbstractMemoryLocationImpl *AML, llvm::ArrayRef<ptrdiff_t> Ind) {

  auto NwLifeTime = AML->lifetime();

  if (NwLifeTime == 0) {
#ifdef XTAINT_DIAGNOSTICS
    overApproximatedAMLs.insert(AML);
#endif
    return AML;
  }

  llvm::SmallVector<ptrdiff_t, 8> Offs(AML->offsets().begin(),
                                       AML->offsets().end());

  bool IsOverApproximating = false;

  if (Ind.empty()) {
    Offs.push_back(0);
    NwLifeTime--;
  } else {
    if (NwLifeTime < Ind.size()) {
      Ind = Ind.slice(0, NwLifeTime);
      IsOverApproximating = true;
    }
    Offs.append(Ind.begin(), Ind.end());

    NwLifeTime -= Ind.size();
  }

  const auto *Ret = getOrCreateImpl(AML->base(), std::move(Offs), NwLifeTime);

#ifdef XTAINT_DIAGNOSTICS
  if (isOverApproximating)
    overApproximatedAMLs.insert(ret);
#endif

  return Ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withOffsetImpl(
    const AbstractMemoryLocationImpl *AML, const llvm::GetElementPtrInst *Gep) {
  assert(DL);

  switch (AML->lifetime()) {
  case 0:
#ifdef XTAINT_DIAGNOSTICS
    overApproximatedAMLs.insert(AML);
#endif
    return AML;
  default:
    auto GepOffs = detail::AbstractMemoryLocationImpl::computeOffset(*DL, Gep);
    if (!GepOffs.has_value()) {
      return limitImpl(AML);
    }
    assert(!AML->offsets().empty() && "An AbstractMemoryLocation should have "
                                      "at least one offset, even if it is 0");
    llvm::SmallVector<ptrdiff_t, 8> Offs(AML->offsets().begin(),
                                         AML->offsets().end());
    Offs.back() += *GepOffs;

    return getOrCreateImpl(AML->base(), std::move(Offs), AML->lifetime());
  }
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withOffsetsImpl(
    const AbstractMemoryLocationImpl *AML, llvm::ArrayRef<ptrdiff_t> Offs) {

  if (Offs.empty()) {
    return AML;
  }

  auto NwLifetime = AML->lifetime();

  if (NwLifetime == 0) {
#ifdef XTAINT_DIAGNOSTICS
    overApproximatedAMLs.insert(AML);
#endif
    return AML;
  }

  assert(!AML->offsets().empty() && "An AbstractMemoryLocation should have "
                                    "at least one offset, even if it is 0");
  llvm::SmallVector<ptrdiff_t, 8> OffsCpy(AML->offsets().begin(),
                                          AML->offsets().end());
  OffsCpy.back() += Offs.front();

  bool IsOverApproximating = false;

  if (NwLifetime < Offs.size() - 1) {
    Offs = Offs.slice(0, NwLifetime + 1);
  }

  OffsCpy.append(std::next(Offs.begin()), Offs.end());

  const auto *Ret = getOrCreateImpl(AML->base(), std::move(OffsCpy),
                                    NwLifetime - Offs.size() + 1);
#ifdef XTAINT_DIAGNOSTICS
  if (isOverApproximating)
    overApproximatedAMLs.insert(ret);
#endif
  return Ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withTransferToImpl(
    const AbstractMemoryLocationImpl *AML,
    const AbstractMemoryLocationImpl *From, const llvm::Value *To) {

  if (AML->lifetime() == 0 && From->lifetime() == 0) {
    const auto *Ret = getOrCreateImpl(To, 0);

#ifdef XTAINT_DIAGNOSTICS
    overApproximatedAMLs.insert(ret);
#endif

    return Ret;
  }

  // already checked that either offsets() is a prefix of From.offsets() or
  // vice versa
  llvm::SmallVector<ptrdiff_t, 8> Offs(
      [&] {
        if (AML->offsets().size() >= From->offsets().size()) {

          if (!From->offsets().empty()) {
            return std::next(AML->offsets().begin(),
                             From->offsets().size() - 1);
          }
          return AML->offsets().begin();
        }
        if (!AML->offsets().empty()) {
          return std::next(From->offsets().begin(), AML->offsets().size() - 1);
        }
        return From->offsets().begin();
      }(),
      [&] {
        return AML->offsets().size() >= From->offsets().size()
                   ? AML->offsets().end()
                   : From->offsets().end();
      }());

  if (!Offs.empty()) {
    Offs.back() = 0;
  }

  return getOrCreateImpl(To, std::move(Offs),
                         std::min(AML->lifetime(), From->lifetime()));
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withTransferFromImpl(
    const AbstractMemoryLocationImpl *AML,
    const AbstractMemoryLocationImpl *To) {

  if (AML->lifetime() == 0) {
#ifdef XTAINT_DIAGNOSTICS
    overApproximatedAMLs.insert(To);
#endif
    return To;
  }

  llvm::SmallVector<ptrdiff_t, 8> Offs(To->offsets().begin(),
                                       To->offsets().end());
  if (!AML->offsets().empty()) {
    if (!Offs.empty()) {
      Offs.back() += AML->offsets()[0];
    }

    Offs.append(std::next(AML->offsets().begin()), AML->offsets().end());
  }

  auto NwLifetime = To->lifetime();
  auto MaximumSize = std::min(AML->offsets().size() + AML->lifetime(),
                              To->offsets().size() + NwLifetime);

  bool IsOverApproximating = false;
  if (Offs.size() > MaximumSize) {
    Offs.resize(MaximumSize);
    NwLifetime = 0;
    IsOverApproximating = true;
  }

  const auto *Ret =
      getOrCreateImpl(To->base(), std::move(Offs),
                      std::min(AML->lifetime(), MaximumSize - Offs.size()));

#ifdef XTAINT_DIAGNOSTICS
  if (isOverApproximating)
    overApproximatedAMLs.insert(ret);
#endif

  return Ret;
}
} // namespace psr::detail