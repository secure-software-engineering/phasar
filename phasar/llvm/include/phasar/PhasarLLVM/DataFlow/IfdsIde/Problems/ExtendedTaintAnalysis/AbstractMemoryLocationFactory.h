/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/TrailingObjects.h"

#include <memory>
#include <vector>

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
    struct Block final : public llvm::TrailingObjects<Block, void *> {

      Block *Next = nullptr;

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
        sizeof(AbstractMemoryLocationImpl) / sizeof(void *);
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
  getOrCreateImpl(const llvm::Value *V, llvm::ArrayRef<ptrdiff_t> Offs,
                  unsigned BOUND);

  const detail::AbstractMemoryLocationImpl *
  getOrCreateImpl(const llvm::Value *V, unsigned BOUND);

  const AbstractMemoryLocationImpl *createImpl(const llvm::Value *V,
                                               unsigned BOUND);

  [[nodiscard]] const AbstractMemoryLocationImpl *getOrCreateZeroImpl() const;

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
    return {limitImpl(AML.operator->())};
  }

public:
  // AbstractMemoryLocationFactory() = default;
  AbstractMemoryLocationFactory(size_t InitialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(InitialCapacity) {}
  AbstractMemoryLocationFactory(const llvm::DataLayout *DL,
                                size_t InitialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(DL, InitialCapacity) {}
  AbstractMemoryLocationFactory(const AbstractMemoryLocationFactory &) = delete;
  ~AbstractMemoryLocationFactory() = default;
  AbstractMemoryLocationFactory &
  operator=(const AbstractMemoryLocationFactory &) = delete;

  [[nodiscard]] AbstractMemoryLocation create(const llvm::Value *V,
                                              unsigned BOUND) {
    return {createImpl(V, BOUND)};
  }

  [[nodiscard]] AbstractMemoryLocation getOrCreateZero() const {
    return {getOrCreateZeroImpl()};
  }

  /// Creates a decendant AbstractMemoryLocation by adding an indirection
  /// (through a store instructon) on AML. The Ind offset-slice can be used to
  /// store indirect taints.
  [[nodiscard]] AbstractMemoryLocation
  withIndirectionOf(const AbstractMemoryLocation &AML,
                    llvm::ArrayRef<ptrdiff_t> Ind) {
    return {withIndirectionOfImpl(AML.operator->(), Ind)};
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffset(const AbstractMemoryLocation &AML,
             const llvm::GetElementPtrInst *Gep) {
    return {withOffsetImpl(AML.operator->(), Gep)};
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffsets(const AbstractMemoryLocation &AML,
              llvm::ArrayRef<ptrdiff_t> Offs) {
    return {withOffsetsImpl(AML.operator->(), Offs)};
  }

  /// Transfers the taint from AML (source at the callsite) seen as From to To
  /// (in the callee). Used for Mapping facts from caller to callee. Assume,
  /// *AML == *From is already checked
  [[nodiscard]] AbstractMemoryLocation
  withTransferTo(const AbstractMemoryLocation &AML,
                 const AbstractMemoryLocation &From, const llvm::Value *To) {
    return {withTransferToImpl(AML.operator->(), From.operator->(), To)};
  }

  /// Transfers the taint from AML (source at the return-site) to To(at the
  /// callsite) where From is the to To corresponding formal-parameter. Used for
  /// mapping facts from callee to caller
  [[nodiscard]] AbstractMemoryLocation
  withTransferFrom(const AbstractMemoryLocation &AML,
                   const AbstractMemoryLocation &To) {
    return {withTransferFromImpl(AML.operator->(), To.operator->())};
  }
};

} // namespace psr

#endif
