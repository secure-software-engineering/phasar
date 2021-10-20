/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H_

#include <iosfwd>

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/IR/Instruction.h" // Need a complete type llvm::Instruction for llvm::PointerIntPair
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"

namespace psr {
class BasicBlockOrdering;
}

namespace psr::XTaint {
class Sanitized {};

class EdgeDomain final {
public:
  enum Kind {
    Bot,
    Top,
    Sanitized,
    WithSanitizer,
  };

private:
  llvm::PointerIntPair<const llvm::Instruction *, 2, Kind> Value;

public:
  inline explicit EdgeDomain() noexcept : Value(nullptr, Kind::Bot) {}

  inline EdgeDomain(std::nullptr_t) noexcept
      : Value(nullptr, Kind::WithSanitizer) {}

  inline EdgeDomain(psr::Bottom) noexcept : Value(nullptr, Kind::Bot) {}

  inline EdgeDomain(psr::Top) noexcept : Value(nullptr, Kind::Top) {}

  inline EdgeDomain(psr::XTaint::Sanitized) noexcept
      : Value(nullptr, Kind::Sanitized) {}

  inline EdgeDomain(const llvm::Instruction *Sani) noexcept
      : Value(Sani, Kind::WithSanitizer) {}

  [[nodiscard]] inline bool isBottom() const { return getKind() == Bot; }
  [[nodiscard]] inline bool isTop() const { return getKind() == Top; }
  [[nodiscard]] inline bool isSanitized() const {
    return getKind() == Sanitized;
  }
  [[nodiscard]] inline bool isNotSanitized() const {
    return getKind() == WithSanitizer && getSanitizer() == nullptr;
  }
  [[nodiscard]] inline bool hasSanitizer() const {
    return getSanitizer() != nullptr;
  }
  [[nodiscard]] inline const llvm::Instruction *getSanitizer() const {
    return Value.getPointer();
  }
  [[nodiscard]] inline bool mayBeSanitized() const {
    return isSanitized() || hasSanitizer();
  }

  [[nodiscard]] inline bool operator==(const EdgeDomain &Other) const {
    return Other.Value == Value;
  }
  [[nodiscard]] inline bool operator!=(const EdgeDomain &Other) const {
    return !(*this == Other);
  }

  /// An arbitrary ordering to be able to insert EdgeDomain values into std::set
  /// and as keys into std::map
  [[nodiscard]] inline bool operator<(const EdgeDomain &Other) const {
    return Other.Value.getOpaqueValue() < Value.getOpaqueValue();
  }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeDomain &ED);
  friend std::ostream &operator<<(std::ostream &OS, const EdgeDomain &ED);

  EdgeDomain join(const EdgeDomain &Other,
                  BasicBlockOrdering *BBO = nullptr) const;

  [[nodiscard]] inline Kind getKind() const { return Value.getInt(); }
};
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H_