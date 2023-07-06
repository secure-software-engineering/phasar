/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H

#include "phasar/Domain/LatticeDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/IR/Instruction.h" // Need a complete type llvm::Instruction for llvm::PointerIntPair
#include "llvm/Support/raw_ostream.h"

namespace psr {
class BasicBlockOrdering;
} // namespace psr

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

  inline EdgeDomain(psr::Bottom /*unused*/) noexcept
      : Value(nullptr, Kind::Bot) {}

  inline EdgeDomain(psr::Top /*unused*/) noexcept : Value(nullptr, Kind::Top) {}

  inline EdgeDomain(psr::XTaint::Sanitized /*unused*/) noexcept
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
  EdgeDomain join(const EdgeDomain &Other,
                  BasicBlockOrdering *BBO = nullptr) const;

  [[nodiscard]] inline Kind getKind() const { return Value.getInt(); }
};
} // namespace psr::XTaint

namespace psr {
template <> struct NonTopBotValue<XTaint::EdgeDomain> {
  using type = const llvm::Instruction *;

  static type unwrap(XTaint::EdgeDomain Value) noexcept {
    return Value.getSanitizer();
  }
};

template <> struct JoinLatticeTraits<XTaint::EdgeDomain> {
  static constexpr auto bottom() noexcept { return Bottom{}; }
  static constexpr auto top() noexcept { return Top{}; }

  static XTaint::EdgeDomain join(ByConstRef<XTaint::EdgeDomain> LHS,
                                 ByConstRef<XTaint::EdgeDomain> RHS) noexcept {
    return LHS.join(RHS);
  }
};
} // namespace psr

#endif
