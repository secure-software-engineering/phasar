/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATION_H_

#include <cstdint>
#include <iosfwd>
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
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

namespace detail {

struct AbstractMemoryLoactionStorage : public llvm::FoldingSetNode {
  const llvm::Value *Baseptr;
  uint32_t Lifetime;
  uint32_t NumOffsets;
  /// The actual length of Offsets is a runtime-constant determined by
  /// NumOffsets. Note, that NumOffsets can be larger than 1
  ptrdiff_t Offsets[1]; // NOLINT

protected:
  AbstractMemoryLoactionStorage(
      const llvm::Value *Baseptr, uint32_t Lifetime,
      const llvm::ArrayRef<ptrdiff_t> &Offsets) noexcept;

  AbstractMemoryLoactionStorage(const llvm::Value *Baseptr,
                                uint32_t Lifetime) noexcept;
};

/// \brief A Memorylocation abstraction represented by a base-pointer and an
/// array of byte offsets.
///
/// The byte offsets are applied to the base-pointer alternating
/// with memory loads in order to represent indirect memory locations in a
/// canonical way.
class AbstractMemoryLocationImpl final : public AbstractMemoryLoactionStorage {

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
  AbstractMemoryLocation(
      const detail::AbstractMemoryLocationImpl *Impl) noexcept;
  inline const detail::AbstractMemoryLocationImpl *operator->() const {
    return PImpl;
  }

  /// Provide an arbitrary partial order for being able to store TaintedValues
  /// in std::set or as key in std::map
  inline bool operator<(const AbstractMemoryLocation &TV) const {
    return PImpl->base() < TV->base();
  }

  inline bool operator==(const AbstractMemoryLocation &AML) const {
    return PImpl == AML.PImpl;
  }

  friend std::ostream &operator<<(std::ostream &OS,
                                  const AbstractMemoryLocation &TV);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AbstractMemoryLocation &TV);

  /// Computes the absolute offset-difference between this and TV assuming,
  /// either this->isProperPrefixOf(TV) or vice versa.
  [[nodiscard]] inline llvm::ArrayRef<ptrdiff_t>
  operator-(const AbstractMemoryLocation &TV) const {
    return *PImpl - *TV.PImpl;
  }

  operator const detail::AbstractMemoryLocationImpl &() const { return *PImpl; }

private:
  const detail::AbstractMemoryLocationImpl *PImpl = nullptr;
};

std::string DToString(const AbstractMemoryLocation &AML);
} // namespace psr

// Hashing support
namespace llvm {

hash_code hash_value(const psr::AbstractMemoryLocation &Val);

template <> struct DenseMapInfo<psr::AbstractMemoryLocation> {
  static inline psr::AbstractMemoryLocation getEmptyKey() {
    return psr::AbstractMemoryLocation(
        DenseMapInfo<psr::detail::AbstractMemoryLocationImpl *>::getEmptyKey());
  }
  static inline psr::AbstractMemoryLocation getTombstoneKey() {
    return psr::AbstractMemoryLocation(
        DenseMapInfo<
            psr::detail::AbstractMemoryLocationImpl *>::getTombstoneKey());
  }
  static unsigned getHashValue(const psr::AbstractMemoryLocation &Val) {
    return hash_value(Val);
  }
  static bool isEqual(const psr::AbstractMemoryLocation &LHS,
                      const psr::AbstractMemoryLocation &RHS) {
    return LHS.operator->() == RHS.operator->();
  }
};

} // namespace llvm

namespace std {
template <> struct hash<psr::AbstractMemoryLocation> {
  size_t operator()(const psr::AbstractMemoryLocation &Val) const {
    return llvm::hash_value(Val);
  }
};

} // namespace std

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ABSTRACTMEMORYLOCATION_H_