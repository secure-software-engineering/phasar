/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Alexander Meinhold and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUE_H

#include <memory>
#include <unordered_set>
#include <variant>

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"

namespace psr {

enum class Ordering { Less, Greater, Equal, Incomparable };

class EdgeValue {
public:
  enum Type { Top, Integer, String, FloatingPoint };

private:
  std::variant<llvm::APInt, llvm::APFloat, std::string, std::nullptr_t>
      ValVariant = nullptr;
  Type VariantType;

public:
  EdgeValue(const llvm::Value *Val);
  EdgeValue(const EdgeValue &EV);
  EdgeValue(llvm::APInt &&VI);
  EdgeValue(const llvm::APInt &VI);
  EdgeValue(llvm::APFloat &&VF);
  EdgeValue(long long VI);
  EdgeValue(int VI);
  EdgeValue(double Double);
  EdgeValue(float Float);
  EdgeValue(std::string &&VS);
  EdgeValue(std::nullptr_t);
  ~EdgeValue();
  const static EdgeValue TopValue;
  [[nodiscard]] bool tryGetInt(uint64_t &Res) const;
  [[nodiscard]] bool tryGetFP(double &Res) const;
  [[nodiscard]] bool tryGetString(std::string &Res) const;
  [[nodiscard]] bool isTop() const;
  [[nodiscard]] bool isNumeric() const;
  [[nodiscard]] bool isString() const;
  [[nodiscard]] Type getKind() const;
  // std::unique_ptr<ObjectLLVM> asObjLLVM(llvm::LLVMContext &ctx) const;
  [[nodiscard]] bool sqSubsetEq(const EdgeValue &Other) const;
  [[nodiscard]] EdgeValue performBinOp(llvm::BinaryOperator::BinaryOps Op,
                                       const EdgeValue &Other) const;
  [[nodiscard]] EdgeValue typecast(Type Dest, unsigned Bits) const;
  EdgeValue &operator=(const EdgeValue &EV);

  operator bool();
  friend bool operator==(const EdgeValue &Lhs, const EdgeValue &Rhs);

  // binary operators
  friend EdgeValue operator+(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator-(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator*(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator/(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator%(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator&(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator|(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator^(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator<<(const EdgeValue &Lhs, const EdgeValue &Rhs);
  friend EdgeValue operator>>(const EdgeValue &Lhs, const EdgeValue &Rhs);
  static int compare(const EdgeValue &Lhs, const EdgeValue &Rhs);

  // unary operators
  EdgeValue operator-() const;
  EdgeValue operator~() const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Os,
                                       const EdgeValue &EV);
  static std::string typeToString(Type Ty);
};
class EdgeValueSet;
using ev_t = EdgeValueSet;

ev_t performBinOp(llvm::BinaryOperator::BinaryOps Op, const ev_t &Lhs,
                  const ev_t &Rhs, size_t MaxSize);
ev_t performTypecast(const ev_t &Ev, EdgeValue::Type Dest, unsigned Bits);
Ordering compare(const ev_t &Lhs, const ev_t &Rhs);
ev_t join(const ev_t &Lhs, const ev_t &Rhs, size_t MaxSize);
/// \brief implements square subset equal
bool operator<(const ev_t &Lhs, const ev_t &Rhs);
bool isTopValue(const ev_t &Val);
llvm::raw_ostream &operator<<(llvm::raw_ostream &Os, const ev_t &Val);

} // namespace psr

namespace std {

template <> struct hash<psr::EdgeValue> {
  hash() = default;
  size_t operator()(const psr::EdgeValue &Val) const {
    auto Hash = hash<int>()(Val.getKind());
    uint64_t AsInt;
    double AsFloat;
    string AsString;
    if (Val.tryGetInt(AsInt)) {
      return hash<uint64_t>()(AsInt) * 31 + Hash;
    }
    if (Val.tryGetFP(AsFloat)) {
      return hash<double>()(round(AsFloat)) * 31 + Hash;
    }
    if (Val.tryGetString(AsString)) {
      return hash<string>()(AsString) * 31 + Hash;
    }
    return Hash;
  }
};

} // namespace std

#endif
