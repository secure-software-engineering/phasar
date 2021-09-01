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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/IFDSAbstractMemoryLocation.h"

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
public:
  using offset_t = AbstractMemoryLocationImpl::offset_t;

private:
  struct Allocator {
    struct Block {
      Block *next = nullptr;
      void *data[0];

      static Block *create(Block *next, size_t numPointerEntries);
      static void destroy(Block *blck);

    private:
      Block(Block *nxt);
    };

    Block *root = nullptr;
    void **pos = nullptr, **end = nullptr;

    Allocator() = default;
    Allocator(size_t initialCapacity);
    ~Allocator();

    AbstractMemoryLocationImpl *create(const llvm::Value *baseptr,
                                       size_t lifetime,
                                       llvm::ArrayRef<offset_t> offsets);

  private:
    constexpr static size_t expectedNumAMLsPerBlock = 1024;
    constexpr static size_t minNumPointersPerAML =
        sizeof(AbstractMemoryLocationImpl) / sizeof(void *);
    constexpr static size_t numPointersPerBlock =
        (minNumPointersPerAML + 3) * expectedNumAMLsPerBlock;
  };

  /// The memory-pool for the impls
  // std::vector<std::unique_ptr<detail::AbstractMemoryLocationImpl>> owner;
  Allocator owner;
  /// The deduplication set for the impls
  llvm::FoldingSet<detail::AbstractMemoryLocationImpl> pool;
  /// The actual cache
  llvm::DenseMap<const llvm::Value *,
                 const detail::AbstractMemoryLocationImpl *>
      cache;

#ifdef XTAINT_DIAGNOSTICS
  llvm::DenseSet<const detail::AbstractMemoryLocationImpl *>
      overApproximatedAMLs;
#endif

protected:
  const llvm::DataLayout *DL = nullptr;

  const detail::AbstractMemoryLocationImpl *
  getOrCreateImpl(const llvm::Value *V, llvm::SmallVectorImpl<offset_t> &&offs,
                  unsigned BOUND);

  const detail::AbstractMemoryLocationImpl *
  getOrCreateImpl(const llvm::Value *V, unsigned BOUND);

  const AbstractMemoryLocationImpl *CreateImpl(const llvm::Value *V,
                                               unsigned BOUND);
  const AbstractMemoryLocationImpl *GetOrCreateZeroImpl() const;
  const AbstractMemoryLocationImpl *
  withIndirectionOfImpl(const AbstractMemoryLocationImpl *AML,
                        llvm::ArrayRef<offset_t> Ind);
  const AbstractMemoryLocationImpl *
  withOffsetImpl(const AbstractMemoryLocationImpl *AML,
                 const llvm::GetElementPtrInst *Gep);

  const AbstractMemoryLocationImpl *
  withOffsetsImpl(const AbstractMemoryLocationImpl *AML,
                  llvm::ArrayRef<offset_t> Offs);

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
  // static inline size_t MaxCreatedSize = 0;

  // AbstractMemoryLocationFactoryBase() = default;
  AbstractMemoryLocationFactoryBase(size_t initialCapacity);
  AbstractMemoryLocationFactoryBase(const llvm::DataLayout *DL,
                                    size_t initialCapacity);
  void setDataLayout(const llvm::DataLayout &DL);

  inline size_t size() const { return pool.size(); }

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
  AbstractMemoryLocationFactory(size_t initialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(initialCapacity) {}
  AbstractMemoryLocationFactory(const llvm::DataLayout *DL,
                                size_t initialCapacity)
      : detail::AbstractMemoryLocationFactoryBase(DL, initialCapacity) {}
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
                    llvm::ArrayRef<offset_t> Ind) {
    return AbstractMemoryLocation(withIndirectionOfImpl(AML.operator->(), Ind));
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffset(const AbstractMemoryLocation &AML,
             const llvm::GetElementPtrInst *Gep) {
    return AbstractMemoryLocation(withOffsetImpl(AML.operator->(), Gep));
  }

  [[nodiscard]] AbstractMemoryLocation
  withOffsets(const AbstractMemoryLocation &AML,
              llvm::ArrayRef<offset_t> Offs) {
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

template <>
class AbstractMemoryLocationFactory<IFDSAbstractMemoryLocation>
    : public detail::AbstractMemoryLocationFactoryBase {

  IFDSAbstractMemoryLocation limit(const IFDSAbstractMemoryLocation &AML) {

    return IFDSAbstractMemoryLocation(limitImpl(AML.operator->()),
                                      AML.getLoadSanitizer());
  }

public:
  // AbstractMemoryLocationFactory() = default;
  AbstractMemoryLocationFactory(const llvm::DataLayout *DL,
                                size_t initialCapacity = 64)
      : detail::AbstractMemoryLocationFactoryBase(DL, initialCapacity) {}
  AbstractMemoryLocationFactory(const IFDSAbstractMemoryLocation &) = delete;

  [[nodiscard]] IFDSAbstractMemoryLocation Create(const llvm::Value *V,
                                                  unsigned BOUND) {
    return IFDSAbstractMemoryLocation(CreateImpl(V, BOUND));
  }
  [[nodiscard]] IFDSAbstractMemoryLocation GetOrCreateZero() const {
    return IFDSAbstractMemoryLocation(GetOrCreateZeroImpl());
  }

  /// Creates a decendant AbstractMemoryLocation by adding an indirection
  /// (through a store instructon) on AML. The Ind offset-slice can be used to
  /// store indirect taints.
  [[nodiscard]] IFDSAbstractMemoryLocation
  withIndirectionOf(const IFDSAbstractMemoryLocation &AML,
                    llvm::ArrayRef<offset_t> Ind) {
    return IFDSAbstractMemoryLocation(
        withIndirectionOfImpl(AML.operator->(), Ind), AML.getLoadSanitizer());
  }

  [[nodiscard]] IFDSAbstractMemoryLocation
  withOffset(const IFDSAbstractMemoryLocation &AML,
             const llvm::GetElementPtrInst *Gep) {
    return IFDSAbstractMemoryLocation(withOffsetImpl(AML.operator->(), Gep),
                                      AML.getLoadSanitizer());
  }

  /// Transfers the taint from AML (source at the callsite) seen as From to To
  /// (in the callee). Used for Mapping facts from caller to callee. Assume,
  /// *AML == *From is already checked
  [[nodiscard]] IFDSAbstractMemoryLocation
  withTransferTo(const IFDSAbstractMemoryLocation &AML,
                 const IFDSAbstractMemoryLocation &From,
                 const llvm::Value *To) {
    return IFDSAbstractMemoryLocation(
        withTransferToImpl(AML.operator->(), From.operator->(), To),
        AML.getLoadSanitizer());
  }

  /// Transfers the taint from AML (source at the return-site) to To(at the
  /// callsite) where From is the to To corresponding formal-parameter. Used for
  /// mapping facts from callee to caller
  [[nodiscard]] IFDSAbstractMemoryLocation
  withTransferFrom(const IFDSAbstractMemoryLocation &AML,
                   const IFDSAbstractMemoryLocation &To) {
    return IFDSAbstractMemoryLocation(
        withTransferFromImpl(AML.operator->(), To.operator->()),
        AML.getLoadSanitizer());
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATIONFACTORY_H_