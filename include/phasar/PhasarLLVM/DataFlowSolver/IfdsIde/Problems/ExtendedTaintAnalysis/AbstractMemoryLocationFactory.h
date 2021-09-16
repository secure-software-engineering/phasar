/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H_

#include <memory>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

namespace llvm {
class DataLayout;
class GetElementPtrInst;
} // namespace llvm

namespace psr {
namespace detail {

/// A factory for detail::AbstractMemoryLocationImpl. Caches all intermedicate
/// objects to allow fast getters/transformators even if they are called
/// multiple times.
class AbstractMemoryLocationFactoryBase {

private:
  struct Allocator {
    struct Block {
      Block *Next = nullptr;
      void *Data[1]; // NOLINT

      static Block *create(Block *Next, size_t NumPointerEntries);
      static void destroy(Block *Blck);

    private:
      Block(Block *Next);
    };

    Block *Root = nullptr;
    void **Pos = nullptr, **End = nullptr;

    Allocator() noexcept = default;
    Allocator(size_t InitialCapacity);
    Allocator(const Allocator &) = delete;
    Allocator(Allocator &&Other) noexcept;
    ~Allocator();

    Allocator &operator=(const Allocator &) = delete;
    Allocator &operator=(Allocator &&Other) noexcept;

    AbstractMemoryLocationImpl *create(const llvm::Value *Baseptr,
                                       size_t Lifetime,
                                       llvm::ArrayRef<ptrdiff_t> Offsets);

  private:
    constexpr static size_t ExpectedNumAmLsPerBlock = 1024;
    constexpr static size_t MinNumPointersPerAML =
        offsetof(AbstractMemoryLocationImpl, Offsets) / sizeof(void *);
    constexpr static size_t NumPointersPerBlock =
        (MinNumPointersPerAML + 3) * ExpectedNumAmLsPerBlock;
  };

  /// The memory-pool for the impls
  Allocator Owner;
  /// The deduplication set for the impls
  llvm::FoldingSet<detail::AbstractMemoryLocationImpl> Pool;
  /// The actual cache
  llvm::DenseMap<const llvm::Value *,
                 const detail::AbstractMemoryLocationImpl *>
      Cache;

#ifdef XTAINT_DIAGNOSTICS
  llvm::DenseSet<const detail::AbstractMemoryLocationImpl *>
      overApproximatedAMLs;
#endif

protected:
  const llvm::DataLayout *DL = nullptr;

  const detail::AbstractMemoryLocationImpl *
  getOrCreateImpl(const llvm::Value *V, llvm::SmallVectorImpl<ptrdiff_t> &&offs,
                  unsigned BOUND);

  const detail::AbstractMemoryLocationImpl *
  getOrCreateImpl(const llvm::Value *V, unsigned BOUND);

  const AbstractMemoryLocationImpl *CreateImpl(const llvm::Value *V,
                                               unsigned BOUND);
  const AbstractMemoryLocationImpl *GetOrCreateZeroImpl() const;
  const AbstractMemoryLocationImpl *
  withIndirectionOfImpl(const AbstractMemoryLocationImpl *AML,
                        llvm::ArrayRef<ptrdiff_t> Ind);
  const AbstractMemoryLocationImpl *
  withOffsetImpl(const AbstractMemoryLocationImpl *AML,
                 const llvm::GetElementPtrInst *Gep);

  const AbstractMemoryLocationImpl *
  withOffsetsImpl(const AbstractMemoryLocationImpl *AML,
                  llvm::ArrayRef<ptrdiff_t> Offs);

  const AbstractMemoryLocationImpl *
  withTransferToImpl(const AbstractMemoryLocationImpl *AML,
                     const AbstractMemoryLocationImpl *From,
                     const llvm::Value *To);
  const AbstractMemoryLocationImpl *
  withTransferFromImpl(const AbstractMemoryLocationImpl *AML,
                       const AbstractMemoryLocationImpl *To);
  const AbstractMemoryLocationImpl *
  limitImpl(const AbstractMemoryLocationImpl *AML);

public:
  AbstractMemoryLocationFactoryBase(size_t InitialCapacity);
  AbstractMemoryLocationFactoryBase(const llvm::DataLayout *DL,
                                    size_t InitialCapacity);
  void setDataLayout(const llvm::DataLayout &DL);

  [[nodiscard]] inline size_t size() const { return Pool.size(); }

#ifdef XTAINT_DIAGNOSTICS
  inline size_t getNumOverApproximatedFacts() const {
    return overApproximatedAMLs.size();
  }
#endif
};
} // namespace detail

template <typename T> class AbstractMemoryLocationFactory;
template <>
class AbstractMemoryLocationFactory<AbstractMemoryLocation>
    : public detail::AbstractMemoryLocationFactoryBase {

  AbstractMemoryLocation limit(const AbstractMemoryLocation &AML) {
    return AbstractMemoryLocation(limitImpl(AML.operator->()));
  }

public:
  // AbstractMemoryLocationFactory() = default;
  AbstractMemoryLocationFactory(size_t InitialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(InitialCapacity) {}
  AbstractMemoryLocationFactory(const llvm::DataLayout *DL,
                                size_t InitialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(DL, InitialCapacity) {}
  AbstractMemoryLocationFactory(const AbstractMemoryLocationFactory &) = delete;

  [[nodiscard]] AbstractMemoryLocation Create(const llvm::Value *V,
                                              unsigned BOUND) {
    return AbstractMemoryLocation(CreateImpl(V, BOUND));
  }
  [[nodiscard]] AbstractMemoryLocation GetOrCreateZero() const {
    return AbstractMemoryLocation(GetOrCreateZeroImpl());
  }

  /// Creates a decendant AbstractMemoryLocation by adding an indirection
  /// (through a store instructon) on AML. The Ind offset-slice can be used to
  /// store indirect taints.
  [[nodiscard]] AbstractMemoryLocation
  withIndirectionOf(const AbstractMemoryLocation &AML,
                    llvm::ArrayRef<ptrdiff_t> Ind) {
    return AbstractMemoryLocation(withIndirectionOfImpl(AML.operator->(), Ind));
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffset(const AbstractMemoryLocation &AML,
             const llvm::GetElementPtrInst *Gep) {
    return AbstractMemoryLocation(withOffsetImpl(AML.operator->(), Gep));
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffsets(const AbstractMemoryLocation &AML,
              llvm::ArrayRef<ptrdiff_t> Offs) {
    return AbstractMemoryLocation(withOffsetsImpl(AML.operator->(), Offs));
  }

  /// Transfers the taint from AML (source at the callsite) seen as From to To
  /// (in the callee). Used for Mapping facts from caller to callee. Assume,
  /// *AML == *From is already checked
  [[nodiscard]] AbstractMemoryLocation
  withTransferTo(const AbstractMemoryLocation &AML,
                 const AbstractMemoryLocation &From, const llvm::Value *To) {
    return AbstractMemoryLocation(
        withTransferToImpl(AML.operator->(), From.operator->(), To));
  }

  /// Transfers the taint from AML (source at the return-site) to To(at the
  /// callsite) where From is the to To corresponding formal-parameter. Used for
  /// mapping facts from callee to caller
  [[nodiscard]] AbstractMemoryLocation
  withTransferFrom(const AbstractMemoryLocation &AML,
                   const AbstractMemoryLocation &To) {
    return AbstractMemoryLocation(
        withTransferFromImpl(AML.operator->(), To.operator->()));
  }
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H_