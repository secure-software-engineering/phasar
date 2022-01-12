/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DOMAIN_EXTENDEDVALUE_H
#define PHASAR_PHASARLLVM_DOMAIN_EXTENDEDVALUE_H

#include <cassert>
#include <functional>
#include <string>
#include <vector>

namespace llvm {

class Value;
} // namespace llvm

namespace psr {

class ExtendedValue {
public:
  ExtendedValue() = default;
  explicit ExtendedValue(const llvm::Value *Val) : Val(Val) {
    assert(Val && "ExtendedValue requires an llvm::Value* object");
  }
  ~ExtendedValue() = default;

  bool operator==(const ExtendedValue &Rhs) const {
    bool IsValueEqual = Val == Rhs.Val;
    if (!IsValueEqual) {
      return false;
    }

    bool IsMemLocationSeqEqual = MemLocationSeq == Rhs.MemLocationSeq;
    if (!IsMemLocationSeqEqual) {
      return false;
    }

    bool IsEndOfTaintedBlockLabelEqual =
        EndOfTaintedBlockLabel == Rhs.EndOfTaintedBlockLabel;
    if (!IsEndOfTaintedBlockLabelEqual) {
      return false;
    }

    bool IsVaListMemLocationSeqEqual =
        VaListMemLocationSeq == Rhs.VaListMemLocationSeq;
    if (!IsVaListMemLocationSeqEqual) {
      return false;
    }

    bool IsVarArgIndexEqual = VarArgIndex == Rhs.VarArgIndex;
    if (!IsVarArgIndexEqual) {
      return false;
    }

    bool IsCurrentVarArgIndexEqual =
        CurrentVarArgIndex == Rhs.CurrentVarArgIndex;
    if (!IsCurrentVarArgIndexEqual) {
      return false;
    }

    return true;
  }

  bool operator<(const ExtendedValue &Rhs) const {
    if (std::less<const llvm::Value *>{}(Val, Rhs.Val)) {
      return true;
    }
    if (std::less<const llvm::Value *>{}(Rhs.Val, Val)) {
      return false;
    }

    if (MemLocationSeq < Rhs.MemLocationSeq) {
      return true;
    }
    if (Rhs.MemLocationSeq < MemLocationSeq) {
      return false;
    }

    if (std::less<std::string>{}(EndOfTaintedBlockLabel,
                                 Rhs.EndOfTaintedBlockLabel)) {
      return true;
    }
    if (std::less<std::string>{}(Rhs.EndOfTaintedBlockLabel,
                                 EndOfTaintedBlockLabel)) {
      return false;
    }

    if (VaListMemLocationSeq < Rhs.VaListMemLocationSeq) {
      return true;
    }
    if (Rhs.VaListMemLocationSeq < VaListMemLocationSeq) {
      return false;
    }

    if (std::less<long>{}(VarArgIndex, Rhs.VarArgIndex)) {
      return true;
    }
    if (std::less<long>{}(Rhs.VarArgIndex, VarArgIndex)) {
      return false;
    }

    return std::less<long>{}(CurrentVarArgIndex, Rhs.CurrentVarArgIndex);
  }

  [[nodiscard]] const llvm::Value *getValue() const { return Val; }

  [[nodiscard]] std::vector<const llvm::Value *> getMemLocationSeq() const {
    return MemLocationSeq;
  }
  void
  setMemLocationSeq(const std::vector<const llvm::Value *> &MemLocationSeq) {
    this->MemLocationSeq = MemLocationSeq;
  }

  [[nodiscard]] std::string getEndOfTaintedBlockLabel() const {
    return EndOfTaintedBlockLabel;
  }
  void setEndOfTaintedBlockLabel(const std::string &EndOfTaintedBlockLabel) {
    this->EndOfTaintedBlockLabel = EndOfTaintedBlockLabel;
  }

  [[nodiscard]] std::vector<const llvm::Value *>
  getVaListMemLocationSeq() const {
    return VaListMemLocationSeq;
  }
  void setVaListMemLocationSeq(
      const std::vector<const llvm::Value *> &VaListMemLocationSeq) {
    this->VaListMemLocationSeq = VaListMemLocationSeq;
  }

  [[nodiscard]] long getVarArgIndex() const { return VarArgIndex; }
  void setVarArgIndex(long VarArgIndex) { this->VarArgIndex = VarArgIndex; }

  void resetVarArgIndex() {
    if (!isVarArgTemplate()) {
      VarArgIndex = -1L;
    }
  }

  [[nodiscard]] long getCurrentVarArgIndex() const {
    return CurrentVarArgIndex;
  }
  void incrementCurrentVarArgIndex() {
    if (!isVarArgTemplate()) {
      ++CurrentVarArgIndex;
    }
  }

  [[nodiscard]] bool isVarArg() const { return VarArgIndex > -1L; }
  [[nodiscard]] bool isVarArgTemplate() const {
    return VaListMemLocationSeq.empty() && isVarArg();
  }

private:
  const llvm::Value *Val = nullptr;
  std::vector<const llvm::Value *> MemLocationSeq;
  std::string EndOfTaintedBlockLabel;

  std::vector<const llvm::Value *> VaListMemLocationSeq;
  long VarArgIndex = -1L;
  long CurrentVarArgIndex = -1L;
};

} // namespace psr

namespace std {

template <> struct hash<psr::ExtendedValue> {
  std::size_t operator()(const psr::ExtendedValue &Ev) const {
    std::size_t Seed = 0x4711;

    Seed ^= hash<const llvm::Value *>{}(Ev.getValue()) + 0x9e3779b9 +
            (Seed << 6) + (Seed >> 2);

    for (const auto &MemLocationPart : Ev.getMemLocationSeq()) {
      Seed ^= hash<const llvm::Value *>{}(MemLocationPart) + 0x9e3779b9 +
              (Seed << 6) + (Seed >> 2);
    }

    Seed ^= hash<string>{}(Ev.getEndOfTaintedBlockLabel()) + 0x9e3779b9 +
            (Seed << 6) + (Seed >> 2);

    for (const auto &VaListMemLocationPart : Ev.getVaListMemLocationSeq()) {
      Seed ^= hash<const llvm::Value *>{}(VaListMemLocationPart) + 0x9e3779b9 +
              (Seed << 6) + (Seed >> 2);
    }

    Seed ^= hash<long>{}(Ev.getVarArgIndex()) + 0x9e3779b9 + (Seed << 6) +
            (Seed >> 2);

    Seed ^= hash<long>{}(Ev.getCurrentVarArgIndex()) + 0x9e3779b9 +
            (Seed << 6) + (Seed >> 2);

    return Seed;
  }
};

} // namespace std

#endif
