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
  explicit EdgeDomain() noexcept;
  EdgeDomain(std::nullptr_t) noexcept;
  EdgeDomain(psr::Bottom) noexcept;
  EdgeDomain(psr::Top) noexcept;
  EdgeDomain(psr::XTaint::Sanitized) noexcept;
  EdgeDomain(const llvm::Instruction *Sani) noexcept;

  [[nodiscard]] bool isBottom() const;
  [[nodiscard]] bool isTop() const;
  [[nodiscard]] bool isSanitized() const;
  [[nodiscard]] bool isNotSanitized() const;
  [[nodiscard]] bool hasSanitizer() const;
  [[nodiscard]] const llvm::Instruction *getSanitizer() const;
  [[nodiscard]] bool mayBeSanitized() const;

  bool operator==(const EdgeDomain &Other) const;
  inline bool operator!=(const EdgeDomain &Other) const {
    return !(*this == Other);
  }
  bool operator<(const EdgeDomain &Other) const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeDomain &ED);
  friend std::ostream &operator<<(std::ostream &OS, const EdgeDomain &ED);

  EdgeDomain join(const EdgeDomain &Other,
                  BasicBlockOrdering *BBO = nullptr) const;

  [[nodiscard]] Kind getKind() const;
};
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H_