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
  llvm::PointerIntPair<const llvm::Instruction *, 2, Kind> value;

public:
  explicit EdgeDomain();
  EdgeDomain(std::nullptr_t);
  EdgeDomain(psr::Bottom);
  EdgeDomain(psr::Top);
  EdgeDomain(psr::XTaint::Sanitized);
  EdgeDomain(const llvm::Instruction *Sani);

  bool isBottom() const;
  bool isTop() const;
  bool isSanitized() const;
  bool isNotSanitized() const;
  bool hasSanitizer() const;
  const llvm::Instruction *getSanitizer() const;
  bool mayBeSanitized() const;

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

  Kind getKind() const;
};
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_EDGEDOMAIN_H_