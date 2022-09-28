/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATION_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/TrailingObjects.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

namespace psr {

namespace detail {

struct AbstractMemoryLocationStorage : public llvm::FoldingSetNode {
  const llvm::Value *Baseptr;
  uint32_t Lifetime;
  uint32_t NumOffsets;

protected:
  AbstractMemoryLocationStorage(const llvm::Value *Baseptr, uint32_t Lifetime,
                                uint32_t NumOffsets = 0) noexcept;
};

/// \brief A Memorylocation abstraction represented by a base-pointer and an
/// array of byte offsets.
///
/// The byte offsets are applied to the base-pointer alternating
/// with memory loads in order to represent indirect memory locations in a
/// canonical way.
class AbstractMemoryLocationImpl final
    : public AbstractMemoryLocationStorage,
      private llvm::TrailingObjects<AbstractMemoryLocationImpl, ptrdiff_t> {

  // The factory is responsible for allocation, so it needs access to the
  // private inherited members from llvm::TrailingObjects
  friend class AbstractMemoryLocationFactoryBase;
  friend TrailingObjects;

  [[nodiscard]] bool
  equivalentOffsets(const AbstractMemoryLocationImpl &TV) const;

public:
  AbstractMemoryLocationImpl(const llvm::Value *Baseptr,
                             unsigned Lifetime) noexcept;
  AbstractMemoryLocationImpl(const llvm::Value *Baseptr,
                             llvm::SmallVectorImpl<ptrdiff_t> &&Offsets,
                             unsigned Lifetime) noexcept;
  AbstractMemoryLocationImpl(const llvm::Value *Baseptr,
                             llvm::ArrayRef<ptrdiff_t> Offsets,
                             unsigned Lifetime) noexcept;
  AbstractMemoryLocationImpl();
  /// Creates the zero-value
  AbstractMemoryLocationImpl(std::nullptr_t) : AbstractMemoryLocationImpl() {}

  AbstractMemoryLocationImpl(const AbstractMemoryLocationImpl &) = delete;
  AbstractMemoryLocationImpl(AbstractMemoryLocationImpl &&) = delete;
  ~AbstractMemoryLocationImpl() = default;

  AbstractMemoryLocationImpl &
  operator=(const AbstractMemoryLocationImpl &) = delete;
  AbstractMemoryLocationImpl &operator=(AbstractMemoryLocationImpl &&) = delete;

  /// Checks whether this AbstractMemoryLocation is the special zero value
  [[nodiscard]] bool isZero() const;
  /// The base pointer
  [[nodiscard]] inline const llvm::Value *base() const { return Baseptr; }
  /// The array of offsets
  [[nodiscard]] llvm::ArrayRef<ptrdiff_t> offsets() const;
  /// The number of modifications that are allowed on this object before
  /// overapproximating
  [[nodiscard]] inline size_t lifetime() const { return Lifetime; }

  /// Compute the byte-offset given by the specified getelementptr instruction.
  ///
  /// \return The byte offset, iff all indices are constants. Otherwise
  /// std::nullopt
  static std::optional<ptrdiff_t>
  computeOffset(const llvm::DataLayout &DL, const llvm::GetElementPtrInst *Gep);

  [[nodiscard]] inline bool isOverApproximation() const {
    return lifetime() == 0;
  }

  // Computes the absolute offset-difference between this and TV assuming,
  // either this->isProperPrefixOf(TV) or vice versa.
  [[nodiscard]] llvm::ArrayRef<ptrdiff_t>
  operator-(const AbstractMemoryLocationImpl &TV) const;
  /// Check whether Larger describes a memory location that is tainted if *this
  /// is tainted, no matter what additional offset is added to Larger. Used to
  /// implement the GEP-FF
  [[nodiscard]] bool
  isProperPrefixOf(const AbstractMemoryLocationImpl &Larger) const;
  [[nodiscard]] bool isProperPrefixOf(
      const AbstractMemoryLocationImpl &Larger,
      PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const;

  /// Are *this and TV equivalent?
  [[nodiscard]] bool equivalent(const AbstractMemoryLocationImpl &TV) const;

  [[nodiscard]] bool
  equivalentExceptPointerArithmetics(const AbstractMemoryLocationImpl &TV,
                                     unsigned PALevel = 1) const;

  /// Are *this and TV equivalent wrt aliasing?
  bool mustAlias(
      const AbstractMemoryLocationImpl &TV,
      PointsToInfo<const llvm::Value *, const llvm::Instruction *> &PT) const;

  /// Provide an arbitrary partial order for being able to store TaintedValues
  /// in std::set or as key in std::map
  inline bool operator<(const AbstractMemoryLocationImpl &TV) const {
    return base() < TV.base();
  }

  inline const AbstractMemoryLocationImpl *operator->() const { return this; }

  // FoldingSet support
  void Profile(llvm::FoldingSetNodeID &ID) const; // NOLINT
  // NOLINTNEXTLINE
  static void MakeProfile(llvm::FoldingSetNodeID &ID, const llvm::Value *V,
                          llvm::ArrayRef<ptrdiff_t> Offs, unsigned Lifetime);
};
} // namespace detail

/// A Wrapper over a pointer to an detail::AbstractMemoryLocationImpl. The impl
/// can be accessed using the -> operator. In contrast to the impl, the equality
/// operators only compare the Impl-references.
///
/// This type can be hashed.
class AbstractMemoryLocation {
public:
  explicit AbstractMemoryLocation() noexcept = default;

  AbstractMemoryLocation(const AbstractMemoryLocation &) noexcept = default;
  AbstractMemoryLocation(AbstractMemoryLocation &&) noexcept = default;
  ~AbstractMemoryLocation() = default;

  AbstractMemoryLocation &
  operator=(const AbstractMemoryLocation &) noexcept = default;
  AbstractMemoryLocation &
  operator=(AbstractMemoryLocation &&) noexcept = default;

  AbstractMemoryLocation(
      const detail::AbstractMemoryLocationImpl *Impl) noexcept;

  inline const detail::AbstractMemoryLocationImpl *operator->() const {
    return PImpl;
  }

  /// Provide an arbitrary partial order for being able to store TaintedValues
  /// in std::set or as key in std::map
  inline bool operator<(AbstractMemoryLocation TV) const {
    return PImpl < TV.PImpl;
  }

  inline bool operator==(AbstractMemoryLocation AML) const {
    return PImpl == AML.PImpl;
  }

  inline bool operator!=(AbstractMemoryLocation AML) const {
    return !(*this == AML);
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       AbstractMemoryLocation TV);

  /// Computes the absolute offset-difference between this and TV assuming,
  /// either this->isProperPrefixOf(TV) or vice versa.
  [[nodiscard]] inline llvm::ArrayRef<ptrdiff_t>
  operator-(AbstractMemoryLocation TV) const {
    return *PImpl - *TV.PImpl;
  }

  operator const detail::AbstractMemoryLocationImpl &() const { return *PImpl; }

private:
  const detail::AbstractMemoryLocationImpl *PImpl = nullptr;
};

// NOLINTNEXTLINE(readability-identifier-naming)
std::string DToString(const AbstractMemoryLocation &AML);

// NOLINTNEXTLINE(readability-identifier-naming)
llvm::hash_code hash_value(psr::AbstractMemoryLocation Val);
} // namespace psr

// Hashing support
namespace llvm {

template <> struct DenseMapInfo<psr::AbstractMemoryLocation> {
  static inline psr::AbstractMemoryLocation getEmptyKey() {
    return {
        DenseMapInfo<psr::detail::AbstractMemoryLocationImpl *>::getEmptyKey()};
  }
  static inline psr::AbstractMemoryLocation getTombstoneKey() {
    return {DenseMapInfo<
        psr::detail::AbstractMemoryLocationImpl *>::getTombstoneKey()};
  }
  static unsigned getHashValue(psr::AbstractMemoryLocation Val) {
    return hash_value(Val);
  }
  static bool isEqual(psr::AbstractMemoryLocation LHS,
                      psr::AbstractMemoryLocation RHS) {
    return LHS.operator->() == RHS.operator->();
  }
};

} // namespace llvm

namespace std {
template <> struct hash<psr::AbstractMemoryLocation> {
  size_t operator()(const psr::AbstractMemoryLocation &Val) const {
    return hash_value(Val);
  }
};

} // namespace std

#endif
