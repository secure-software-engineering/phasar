/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_IFDSABSTRACTMEMORYLOCATION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_IFDSABSTRACTMEMORYLOCATION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

namespace psr {
class IFDSAbstractMemoryLocation : public AbstractMemoryLocation {
  const llvm::Instruction *loadSanitizer = nullptr;

public:
  explicit IFDSAbstractMemoryLocation() = default;
  IFDSAbstractMemoryLocation(const detail::AbstractMemoryLocationImpl *Impl,
                             const llvm::Instruction *Sani = nullptr);
  [[nodiscard]] inline const llvm::Instruction *getLoadSanitizer() const {
    return loadSanitizer;
  }

  /// This tainted AbstractMemoryLocation is already sanitized. Thus, upcoming
  /// loads on this do not result in a tainted value.
  [[nodiscard]] bool isLoadSanitized(const llvm::Instruction *CurrInst) const;

  [[nodiscard]] IFDSAbstractMemoryLocation
  withSanitizedLoad(const llvm::Instruction *Sani) const;

  inline bool operator==(const IFDSAbstractMemoryLocation &AML) const {
    return pImpl == AML.pImpl && loadSanitizer == AML.loadSanitizer;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const IFDSAbstractMemoryLocation &TV);
};

std::string DToString(const IFDSAbstractMemoryLocation &AML);
} // namespace psr

// Hashing support
namespace llvm {

hash_code hash_value(const psr::IFDSAbstractMemoryLocation &Val);

template <> struct DenseMapInfo<psr::IFDSAbstractMemoryLocation> {
  static inline psr::IFDSAbstractMemoryLocation getEmptyKey() {
    return psr::IFDSAbstractMemoryLocation(
        DenseMapInfo<psr::detail::AbstractMemoryLocationImpl *>::getEmptyKey());
  }
  static inline psr::IFDSAbstractMemoryLocation getTombstoneKey() {
    return psr::IFDSAbstractMemoryLocation(
        DenseMapInfo<
            psr::detail::AbstractMemoryLocationImpl *>::getTombstoneKey());
  }
  static unsigned getHashValue(const psr::IFDSAbstractMemoryLocation &Val) {
    return hash_value(Val);
  }
  static bool isEqual(const psr::IFDSAbstractMemoryLocation &LHS,
                      const psr::IFDSAbstractMemoryLocation &RHS) {
    return LHS.operator->() == RHS.operator->() &&
           LHS.getLoadSanitizer() == RHS.getLoadSanitizer();
  }
};

} // namespace llvm

namespace std {
template <> struct hash<psr::IFDSAbstractMemoryLocation> {
  size_t operator()(const psr::IFDSAbstractMemoryLocation &Val) const {
    return llvm::hash_value(Val);
  }
};

} // namespace std

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_IFDSABSTRACTMEMORYLOCATION_H_