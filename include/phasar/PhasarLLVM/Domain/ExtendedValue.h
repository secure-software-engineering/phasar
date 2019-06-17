/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef EXTENDEDVALUE_H
#define EXTENDEDVALUE_H

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
  ExtendedValue() {}
  explicit ExtendedValue(const llvm::Value *_value) : value(_value) {
    assert(value && "ExtendedValue requires an llvm::Value* object");
  }
  ~ExtendedValue() = default;

  bool operator==(const ExtendedValue &rhs) const {
    bool isValueEqual = value == rhs.value;
    if (!isValueEqual)
      return false;

    bool isMemLocationSeqEqual = memLocationSeq == rhs.memLocationSeq;
    if (!isMemLocationSeqEqual)
      return false;

    bool isEndOfTaintedBlockLabelEqual =
        endOfTaintedBlockLabel == rhs.endOfTaintedBlockLabel;
    if (!isEndOfTaintedBlockLabelEqual)
      return false;

    bool isVaListMemLocationSeqEqual =
        vaListMemLocationSeq == rhs.vaListMemLocationSeq;
    if (!isVaListMemLocationSeqEqual)
      return false;

    bool isVarArgIndexEqual = varArgIndex == rhs.varArgIndex;
    if (!isVarArgIndexEqual)
      return false;

    bool isCurrentVarArgIndexEqual =
        currentVarArgIndex == rhs.currentVarArgIndex;
    if (!isCurrentVarArgIndexEqual)
      return false;

    return true;
  }

  bool operator<(const ExtendedValue &rhs) const {
    if (std::less<const llvm::Value *>{}(value, rhs.value))
      return true;
    if (std::less<const llvm::Value *>{}(rhs.value, value))
      return false;

    if (memLocationSeq < rhs.memLocationSeq)
      return true;
    if (rhs.memLocationSeq < memLocationSeq)
      return false;

    if (std::less<std::string>{}(endOfTaintedBlockLabel,
                                 rhs.endOfTaintedBlockLabel))
      return true;
    if (std::less<std::string>{}(rhs.endOfTaintedBlockLabel,
                                 endOfTaintedBlockLabel))
      return false;

    if (vaListMemLocationSeq < rhs.vaListMemLocationSeq)
      return true;
    if (rhs.vaListMemLocationSeq < vaListMemLocationSeq)
      return false;

    if (std::less<long>{}(varArgIndex, rhs.varArgIndex))
      return true;
    if (std::less<long>{}(rhs.varArgIndex, varArgIndex))
      return false;

    return std::less<long>{}(currentVarArgIndex, rhs.currentVarArgIndex);
  }

  const llvm::Value *getValue() const { return value; }

  const std::vector<const llvm::Value *> getMemLocationSeq() const {
    return memLocationSeq;
  }
  void setMemLocationSeq(std::vector<const llvm::Value *> _memLocationSeq) {
    memLocationSeq = _memLocationSeq;
  }

  const std::string getEndOfTaintedBlockLabel() const {
    return endOfTaintedBlockLabel;
  }
  void setEndOfTaintedBlockLabel(std::string _endOfTaintedBlockLabel) {
    endOfTaintedBlockLabel = _endOfTaintedBlockLabel;
  }

  const std::vector<const llvm::Value *> getVaListMemLocationSeq() const {
    return vaListMemLocationSeq;
  }
  void setVaListMemLocationSeq(
      std::vector<const llvm::Value *> _vaListMemLocationSeq) {
    vaListMemLocationSeq = _vaListMemLocationSeq;
  }

  long getVarArgIndex() const { return varArgIndex; }
  void setVarArgIndex(long _varArgIndex) { varArgIndex = _varArgIndex; }

  void resetVarArgIndex() {
    if (!isVarArgTemplate())
      varArgIndex = -1L;
  }

  long getCurrentVarArgIndex() const { return currentVarArgIndex; }
  void incrementCurrentVarArgIndex() {
    if (!isVarArgTemplate())
      ++currentVarArgIndex;
  }

  bool isVarArg() const { return varArgIndex > -1L; }
  bool isVarArgTemplate() const {
    return vaListMemLocationSeq.empty() && isVarArg();
  }

private:
  const llvm::Value *value = nullptr;
  std::vector<const llvm::Value *> memLocationSeq;
  std::string endOfTaintedBlockLabel;

  std::vector<const llvm::Value *> vaListMemLocationSeq;
  long varArgIndex = -1L;
  long currentVarArgIndex = -1L;
};

} // namespace psr

namespace std {

template <> struct hash<psr::ExtendedValue> {
  std::size_t operator()(const psr::ExtendedValue &ev) const {
    std::size_t seed = 0x4711;

    seed ^= hash<const llvm::Value *>{}(ev.getValue()) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);

    for (const auto &memLocationPart : ev.getMemLocationSeq()) {
      seed ^= hash<const llvm::Value *>{}(memLocationPart) + 0x9e3779b9 +
              (seed << 6) + (seed >> 2);
    }

    seed ^= hash<string>{}(ev.getEndOfTaintedBlockLabel()) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);

    for (const auto &vaListMemLocationPart : ev.getVaListMemLocationSeq()) {
      seed ^= hash<const llvm::Value *>{}(vaListMemLocationPart) + 0x9e3779b9 +
              (seed << 6) + (seed >> 2);
    }

    seed ^= hash<long>{}(ev.getVarArgIndex()) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);

    seed ^= hash<long>{}(ev.getCurrentVarArgIndex()) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);

    return seed;
  }
};

} // namespace std

#endif // EXTENDEDVALUE_H
