/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"
#include "phasar/Utils/Logger.h"

namespace psr {
namespace detail {

AbstractMemoryLocationFactoryBase::Allocator::Block::Block(Block *next)
    : next(next) {}

auto AbstractMemoryLocationFactoryBase::Allocator::Block::create(
    Block *next, size_t numPointerEntries) -> Block * {
  // Allocate one more pointer to store the next-block ptr
  auto ret = reinterpret_cast<Block *>(new size_t[1 + numPointerEntries]);

  new (ret) Block(next);
  return ret;
}

void AbstractMemoryLocationFactoryBase::Allocator::Block::destroy(Block *blck) {
  delete[] reinterpret_cast<size_t *>(blck);
}

AbstractMemoryLocationFactoryBase::Allocator::Allocator(
    size_t initialCapacity) {
  if (initialCapacity <= expectedNumAMLsPerBlock)
    return;

  const auto numPointersPerInitialBlock =
      (minNumPointersPerAML + 3) * initialCapacity;
  root = Block::create(nullptr, numPointersPerInitialBlock);
  pos = root->data;
  end = pos + numPointersPerInitialBlock;
}

AbstractMemoryLocationFactoryBase::Allocator::~Allocator() {
  auto blck = root;
  while (blck) {
    auto nxt = blck->next;
    Block::destroy(blck);
    blck = nxt;
  }
}

AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::Allocator::create(
    const llvm::Value *baseptr, size_t lifetime,
    llvm::ArrayRef<offset_t> offsets) {

  // All fields inside AML have pointer size, so there is no padding at all
  auto numPointersRequired = minNumPointersPerAML + offsets.size();
  auto rt = root;
  auto curr = pos;

  if (end - curr < ptrdiff_t(numPointersRequired)) {
    root = rt = Block::create(rt, numPointersPerBlock);
    pos = curr = rt->data;
    end = curr + numPointersPerBlock;
  }

  auto ret = reinterpret_cast<AbstractMemoryLocationImpl *>(curr);

  pos += numPointersRequired;

  new (ret) AbstractMemoryLocationImpl(baseptr, offsets, lifetime);

  return ret;
}

AbstractMemoryLocationFactoryBase::AbstractMemoryLocationFactoryBase(
    size_t initialCapacity)
    : owner(initialCapacity), DL(nullptr) {

  pool.reserve(initialCapacity);
  cache.reserve(initialCapacity);
}

AbstractMemoryLocationFactoryBase::AbstractMemoryLocationFactoryBase(
    const llvm::DataLayout *DL, size_t initialCapacity)
    : owner(initialCapacity), DL(DL) {
  assert(DL);

  pool.reserve(initialCapacity);
  cache.reserve(initialCapacity);
}

void AbstractMemoryLocationFactoryBase::setDataLayout(
    const llvm::DataLayout &DL) {
  assert(!this->DL);
  this->DL = &DL;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::getOrCreateImpl(
    const llvm::Value *V, llvm::SmallVectorImpl<offset_t> &&offs,
    unsigned BOUND) {
  llvm::FoldingSetNodeID ID;
  detail::AbstractMemoryLocationImpl::MakeProfile(ID, V, offs, BOUND);
  void *pos;
  auto *mem = pool.FindNodeOrInsertPos(ID, pos);
  if (!mem) {
    mem = owner.create(V, BOUND, offs);
    pool.InsertNode(mem, pos);

    // if (pool.size() % 10000 == 0) {
    //   std::cerr << "Allocated " << pool.size() << " dataflow facts so far\n";
    //   std::cerr << "Maximum requested field-access-path length so far: "
    //             << MaxCreatedSize << "\n";
    //   std::cerr << "Current: " << AbstractMemoryLocation(mem) << "\n";
    //   std::cerr <<
    //   "-----------------------------------------------------------"
    //                "---------------------";
    // }
  }
  return mem;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::getOrCreateImpl(const llvm::Value *V,
                                                   unsigned BOUND) {

  llvm::SmallVector<offset_t, 1> offs = {0};
  auto ret = getOrCreateImpl(V, std::move(offs), BOUND == 0 ? 0 : BOUND - 1);

  // std::cerr << "Create(" << llvmIRToShortString(V) << ", BOUND=" << BOUND
  //           << ") = " << AbstractMemoryLocation(ret) << "\n";

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::CreateImpl(const llvm::Value *V,
                                              unsigned BOUND) {
  assert(DL);
  if (auto it = cache.find(V); it != cache.end()) {
    return it->second;
  }

  if (llvm::isa<llvm::GlobalValue>(V) || llvm::isa<llvm::AllocaInst>(V) ||
      llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::CallBase>(V)) {
    // Globals, argument, function calls and allocas define themselves

    auto *mem = getOrCreateImpl(V, BOUND);

    cache[V] = mem;

    return mem;
  }

  llvm::SmallVector<offset_t, 8> offs = {0};
  unsigned ver = 1;
  // recursive lambda via Y-combinator
  const auto createRec =
      [this, BOUND](auto &_createRec, llvm::SmallVectorImpl<offset_t> &offs,
                    unsigned &ver,
                    const llvm::Value *V) -> const llvm::Value * {
    if (auto load = llvm::dyn_cast<llvm::LoadInst>(V)) {
      offs.push_back(0);
      ++ver;
      return _createRec(_createRec, offs, ver, load->getPointerOperand());
    }
    if (auto cast = llvm::dyn_cast<llvm::CastInst>(V)) {
      return _createRec(_createRec, offs, ver, cast->getOperand(0));
    }
    if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(V)) {

      auto gepOffs =
          detail::AbstractMemoryLocationImpl::computeOffset(*DL, gep);
      if (gepOffs.has_value()) {
        offs.back() += *gepOffs;
        // ++ver;
      } else {
        // everything until now is irrelevant (constructing the offs-vector in
        // reverse order)
        offs.clear();
        offs.push_back(0);
        ver = BOUND;
      }
      return _createRec(_createRec, offs, ver, gep->getPointerOperand());
    }
    // TODO aggregate instructions, e.g. insertvalue, extractvalue, ...

    return V;
  };
  auto baseptr = createRec(createRec, offs, ver, V);
  std::reverse(offs.begin(), offs.end());
  auto lifetime = BOUND - std::min(ver, BOUND);

  // assert(ver >= offs.size());

  bool isOverApproximating = false;

  if (offs.size() > BOUND) {
    assert(lifetime == 0);

    // if (offs.size() > MaxCreatedSize)
    //   MaxCreatedSize = offs.size();
    isOverApproximating = true;
    offs.resize(BOUND);
  }

  auto *mem = getOrCreateImpl(baseptr, std::move(offs), lifetime);

#ifdef XTAINT_DIAGNOSTICS
  if (isOverApproximating)
    overApproximatedAMLs.insert(mem);
#endif

  // std::cerr << "Create(" << llvmIRToShortString(V)
  //           << ") = " << AbstractMemoryLocation(mem) << "\n";

  cache[V] = mem;
  return mem;
}

[[nodiscard]] const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::GetOrCreateZeroImpl() const {
  // Can allocate without Allocator, because the number of offsets is zero
  static detail::AbstractMemoryLocationImpl zero = nullptr;
  return &zero;
}

const AbstractMemoryLocationImpl *AbstractMemoryLocationFactoryBase::limitImpl(
    const AbstractMemoryLocationImpl *AML) {
  auto beg = AML->offsets().begin();
  auto *end = AML->offsets().end();
  llvm::SmallVector<offset_t, 8> offs(beg, beg == end ? end : end - 1);
  auto *ret = getOrCreateImpl(AML->base(), std::move(offs), 0);

#ifdef XTAINT_DIAGNOSTICS
  overApproximatedAMLs.insert(ret);
#endif

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withIndirectionOfImpl(
    const AbstractMemoryLocationImpl *AML, llvm::ArrayRef<offset_t> Ind) {
  auto ret = [&] {
    auto nwLifeTime = AML->lifetime();

    if (nwLifeTime == 0) {
#ifdef XTAINT_DIAGNOSTICS
      overApproximatedAMLs.insert(AML);
#endif
      return AML;
    }

    llvm::SmallVector<offset_t, 8> offs(AML->offsets().begin(),
                                        AML->offsets().end());

    bool isOverApproximating = false;

    if (Ind.empty()) {
      offs.push_back(0);
      nwLifeTime--;
    } else {
      if (nwLifeTime < Ind.size()) {
        Ind = Ind.slice(0, nwLifeTime);
        isOverApproximating = true;
      }
      offs.append(Ind.begin(), Ind.end());

      nwLifeTime -= Ind.size();
    }

    auto *ret = getOrCreateImpl(AML->base(), std::move(offs), nwLifeTime);

#ifdef XTAINT_DIAGNOSTICS
    if (isOverApproximating)
      overApproximatedAMLs.insert(ret);
#endif

    return ret;
  }();

  // std::cerr << "WithIndirectionOf(" << AbstractMemoryLocation(AML) << ", "
  //           << SequencePrinter(Ind) << ") = " << AbstractMemoryLocation(ret)
  //           << "\n";

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withOffsetImpl(
    const AbstractMemoryLocationImpl *AML, const llvm::GetElementPtrInst *Gep) {
  assert(DL);
  auto ret = [&] {
    switch (AML->lifetime()) {
      /* case 1:
         return limitImpl(AML);*/
    case 0:
#ifdef XTAINT_DIAGNOSTICS
      overApproximatedAMLs.insert(AML);
#endif
      return AML;
    default:
      auto gepOffs =
          detail::AbstractMemoryLocationImpl::computeOffset(*DL, Gep);
      if (!gepOffs.has_value()) {
        return limitImpl(AML);
      }
      assert(!AML->offsets().empty());
      llvm::SmallVector<offset_t, 8> offs(AML->offsets().begin(),
                                          AML->offsets().end());
      offs.back() += *gepOffs;

      return getOrCreateImpl(AML->base(), std::move(offs),
                             AML->lifetime() /*- 1*/);
    }
  }();

  // std::cerr << "WithOffset(" << AbstractMemoryLocation(AML) << ", "
  //           << llvmIRToString(Gep) << ") = " << AbstractMemoryLocation(ret)
  //           << "\n";

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withOffsetsImpl(
    const AbstractMemoryLocationImpl *AML, llvm::ArrayRef<offset_t> Offs) {
  auto ret = [&] {
    if (Offs.empty())
      return AML;

    auto nwLifetime = AML->lifetime();
    switch (nwLifetime) {
    /*case 1:
      return limitImpl(AML);*/
    case 0:
#ifdef XTAINT_DIAGNOSTICS
      overApproximatedAMLs.insert(AML);
#endif
      return AML;
    default:
      assert(!AML->offsets().empty());
      llvm::SmallVector<offset_t, 8> offsCpy(AML->offsets().begin(),
                                             AML->offsets().end());
      offsCpy.back() += Offs.front();

      bool isOverApproximating = false;

      if (nwLifetime < Offs.size() - 1) {
        Offs = Offs.slice(0, nwLifetime + 1);
      }

      offsCpy.append(std::next(Offs.begin()), Offs.end());

      auto *ret = getOrCreateImpl(AML->base(), std::move(offsCpy),
                                  nwLifetime - Offs.size() + 1);
#ifdef XTAINT_DIAGNOSTICS
      if (isOverApproximating)
        overApproximatedAMLs.insert(ret);
#endif
      return ret;
    }
  }();

  // std::cerr << "WithOffsets(" << AbstractMemoryLocation(AML) << ", "
  //           << SequencePrinter(Offs) << ") = " << AbstractMemoryLocation(ret)
  //           << "\n";

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withTransferToImpl(
    const AbstractMemoryLocationImpl *AML,
    const AbstractMemoryLocationImpl *From, const llvm::Value *To) {
  auto ret = [&] {
    if (AML->lifetime() == 0 && From->lifetime() == 0) {
      auto *ret = getOrCreateImpl(To, 0);

#ifdef XTAINT_DIAGNOSTICS
      overApproximatedAMLs.insert(ret);
#endif

      return ret;
    }

    // already checked that either offsets() is a prefix of From.offsets() or
    // vice versa
    llvm::SmallVector<offset_t, 8> offs(
        [&] {
          if (AML->offsets().size() >= From->offsets().size()) {

            if (!From->offsets().empty())
              return std::next(AML->offsets().begin(),
                               From->offsets().size() - 1);
            return AML->offsets().begin();
          } else {
            if (!AML->offsets().empty())
              return std::next(From->offsets().begin(),
                               AML->offsets().size() - 1);
            return From->offsets().begin();
          }
        }(),
        [&] {
          return AML->offsets().size() >= From->offsets().size()
                     ? AML->offsets().end()
                     : From->offsets().end();
        }());
    if (offs.size())
      offs.back() = 0;

    return getOrCreateImpl(To, std::move(offs),
                           std::min(AML->lifetime(), From->lifetime()));
  }();

  // std::cerr << "WithTransferTo(" << AbstractMemoryLocation(AML)
  //           << ", from: " << AbstractMemoryLocation(From)
  //           << ", to: " << llvmIRToShortString(To)
  //           << ") = " << AbstractMemoryLocation(ret) << "\n";

  return ret;
}

const AbstractMemoryLocationImpl *
AbstractMemoryLocationFactoryBase::withTransferFromImpl(
    const AbstractMemoryLocationImpl *AML,
    const AbstractMemoryLocationImpl *To) {
  auto ret = [&] {
    if (AML->lifetime() == 0) {
#ifdef XTAINT_DIAGNOSTICS
      overApproximatedAMLs.insert(To);
#endif
      return To;
    }

    llvm::SmallVector<offset_t, 8> offs(To->offsets().begin(),
                                        To->offsets().end());
    if (AML->offsets().size()) {
      if (offs.size())
        offs.back() += AML->offsets()[0];

      offs.append(std::next(AML->offsets().begin()), AML->offsets().end());
    }

    auto nwLifetime = To->lifetime();
    auto maximumSize = std::min(AML->offsets().size() + AML->lifetime(),
                                To->offsets().size() + nwLifetime);

    bool isOverApproximating = false;
    if (offs.size() > maximumSize) {
      offs.resize(maximumSize);
      nwLifetime = 0;
      isOverApproximating = true;
    }

    auto *ret =
        getOrCreateImpl(To->base(), std::move(offs),
                        std::min(AML->lifetime(), maximumSize - offs.size()));

#ifdef XTAINT_DIAGNOSTICS
    if (isOverApproximating)
      overApproximatedAMLs.insert(ret);
#endif

    return ret;
  }();

  // std::cerr << "WithTransferFrom(" << AbstractMemoryLocation(AML)
  //           << ", to: " << AbstractMemoryLocation(To)
  //           << ") = " << AbstractMemoryLocation(ret) << "\n";

  return ret;
}
} // namespace detail
} // namespace psr