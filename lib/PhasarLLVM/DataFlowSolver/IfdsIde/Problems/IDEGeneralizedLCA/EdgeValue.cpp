/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Alexander Meinhold and others
 *****************************************************************************/

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"

namespace psr {

std::ostream &printSemantics(const llvm::APFloat &Fl) {
  if (&Fl.getSemantics() == &llvm::APFloat::IEEEdouble()) {
    return std::cout << "IEEEdouble";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::IEEEhalf()) {
    return std::cout << "IEEEhalf";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::IEEEquad()) {
    return std::cout << "IEEEquad";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::IEEEsingle()) {
    return std::cout << "IEEEsingle";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::PPCDoubleDouble()) {
    return std::cout << "PPCDoubleDouble";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::x87DoubleExtended()) {
    return std::cout << "x87DoubleExtended";
  }
  if (&Fl.getSemantics() == &llvm::APFloat::Bogus()) {
    return std::cout << "Bogus";
  }
  return std::cout << "Sth else";
}

const EdgeValue EdgeValue::TopValue = EdgeValue(nullptr);

EdgeValue::EdgeValue(const llvm::Value *Val) : VariantType(Top) {
  if (const auto *Const = llvm::dyn_cast<llvm::Constant>(Val)) {
    if (Const->getType()->isIntegerTy()) {
      VariantType = Integer;
      ValVariant =
          llvm::APInt(llvm::cast<llvm::ConstantInt>(Const)->getValue());
    } else if (Const->getType()->isFloatingPointTy()) {
      VariantType = FloatingPoint;
      const auto &ConstFP = llvm::cast<llvm::ConstantFP>(Const)->getValueAPF();

      llvm::APFloat Apf(ConstFP);
      bool Unused;
      Apf.convert(llvm::APFloat::IEEEdouble(),
                  llvm::APFloat::roundingMode::NearestTiesToEven, &Unused);
      ValVariant = llvm::APFloat(Apf);
    } else if (llvm::isa<llvm::ConstantPointerNull>(Const)) {
      VariantType = String;
      ValVariant = std::string();
    } else if (Const->getType()->isPointerTy() &&
               Const->getType()->getPointerElementType()->isIntegerTy()) {
      VariantType = String;
      const auto *Gep = llvm::cast<llvm::ConstantExpr>(
          Const); // already checked, hence cast instead of dyn_cast
      if (const auto *Glob =
              llvm::dyn_cast<llvm::GlobalVariable>(Gep->getOperand(0))) {
        ValVariant = std::string(
            llvm::cast<llvm::ConstantDataArray>(Glob->getInitializer())
                ->getAsCString()
                .str());
      } else {
        // inttoptr
        ValVariant = nullptr;
        VariantType = Top;
      }
    } else {
      Val = nullptr;
      VariantType = Top;
    }
  } else {
    Val = nullptr;
    VariantType = Top;
  }
}

EdgeValue::EdgeValue(const EdgeValue &Ev) : VariantType(Ev.VariantType) {
  switch (VariantType) {
  case Top:
    ValVariant = nullptr;
    break;
  case Integer:
    ValVariant = std::get<llvm::APInt>(Ev.ValVariant);
    break;
  case FloatingPoint:
    ValVariant = std::get<llvm::APFloat>(Ev.ValVariant);
    break;
  case String:
    ValVariant = std::get<std::string>(Ev.ValVariant);
    break;
  }
}

EdgeValue &EdgeValue::operator=(const EdgeValue &Ev) {
  this->~EdgeValue();
  new (this) EdgeValue(Ev);
  return *this;
}

EdgeValue::~EdgeValue() { ValVariant.~variant(); }

EdgeValue::EdgeValue(llvm::APInt &&Vi) : VariantType(EdgeValue::Integer) {
  ValVariant = llvm::APInt(std::move(Vi));
}

EdgeValue::EdgeValue(const llvm::APInt &Vi) : VariantType(EdgeValue::Integer) {
  ValVariant = llvm::APInt(Vi);
}

EdgeValue::EdgeValue(llvm::APFloat &&Vf)
    : VariantType(EdgeValue::FloatingPoint) {
  llvm::APFloat Fp = llvm::APFloat(std::move(Vf));
  bool Unused;
  Fp.convert(llvm::APFloat::IEEEdouble(),
             llvm::APFloat::roundingMode::NearestTiesToEven, &Unused);
  ValVariant = Fp;
}

EdgeValue::EdgeValue(long long Vi) : VariantType(EdgeValue::Integer) {
  ValVariant = llvm::APInt(llvm::APInt(sizeof(long long) << 3, Vi));
}

EdgeValue::EdgeValue(int Vi) : VariantType(EdgeValue::Integer) {
  ValVariant = llvm::APInt(llvm::APInt(sizeof(int) << 3, Vi));
}

EdgeValue::EdgeValue(double Double) : VariantType(EdgeValue::FloatingPoint) {
  ValVariant = llvm::APFloat(Double);
}

EdgeValue::EdgeValue(float Float) : VariantType(EdgeValue::FloatingPoint) {
  ValVariant = llvm::APFloat(Float);
}

EdgeValue::EdgeValue(std::string &&Vs) : VariantType(EdgeValue::String) {
  ValVariant = std::string(Vs);
}

EdgeValue::EdgeValue(std::nullptr_t) : VariantType(EdgeValue::Top) {}
bool EdgeValue::tryGetInt(uint64_t &Res) const {
  if (VariantType != Integer) {
    return false;
  }
  Res = std::get<llvm::APInt>(ValVariant).getLimitedValue();
  return true;
}

bool EdgeValue::tryGetFP(double &Res) const {
  if (VariantType != FloatingPoint) {
    return false;
  }
  Res = std::get<llvm::APFloat>(ValVariant).convertToDouble();
  return true;
}

bool EdgeValue::tryGetString(std::string &Res) const {
  if (VariantType != String) {
    return false;
  }
  Res = std::get<std::string>(ValVariant);
  return true;
}

bool EdgeValue::isTop() const { return VariantType == Top; }

bool EdgeValue::isNumeric() const {
  return VariantType == Integer || VariantType == FloatingPoint;
}

bool EdgeValue::isString() const { return VariantType == String; }

EdgeValue::Type EdgeValue::getKind() const { return VariantType; }

EdgeValue::operator bool() {
  switch (VariantType) {
  case Integer:
    return !std::get<llvm::APInt>(ValVariant).isNullValue();
  case FloatingPoint:
    return std::get<llvm::APFloat>(ValVariant).isNonZero();
  case String:
    return !std::get<std::string>(ValVariant).empty();
  default:
    break;
  }
  return false;
}

bool operator==(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  // std::cout << "Compare edge values" << std::endl;
  if (Lhs.VariantType != Rhs.VariantType) {
    // std::cout << "Comparing incompatible types" << std::endl;
    return false;
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Top:
    return true;
  case EdgeValue::Integer:
    // if (v1.value.asInt != v2.value.asInt)
    //  std::cout << "integer unequal" << std::endl;
    return std::get<llvm::APInt>(Lhs.ValVariant) ==
           std::get<llvm::APInt>(Rhs.ValVariant);
  case EdgeValue::FloatingPoint: {
    // std::cout << "compare floating points" << std::endl;
    auto Cp = std::get<llvm::APFloat>(Lhs.ValVariant)
                  .compare(std::get<llvm::APFloat>(Rhs.ValVariant));
    if (Cp == llvm::APFloat::cmpResult::cmpEqual) {
      // std::cout << "FP equal" << std::endl;
      return true;
    }
    auto D1 = std::get<llvm::APFloat>(Lhs.ValVariant).convertToDouble();
    auto D2 = std::get<llvm::APFloat>(Rhs.ValVariant).convertToDouble();

    const double Epsilon = 0.000001;
    // std::cout << "Compare " << d1 << " against " << d2 << std::endl;
    return D1 == D2 || D1 - D2 < Epsilon || D2 - D1 < Epsilon;
  }
  case EdgeValue::String:
    // if (v1.value.asString != v2.value.asString)
    //  std::cout << "String unequal" << std::endl;
    return std::get<std::string>(Lhs.ValVariant) ==
           std::get<std::string>(Rhs.ValVariant);
  default: // will not happen
    std::cerr << "FATAL ERROR" << std::endl;
    return false;
  }
}

bool EdgeValue::sqSubsetEq(const EdgeValue &Other) const {
  return Other.isTop() || Other.VariantType == VariantType;
}

// binary operators
EdgeValue operator+(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) +
            std::get<llvm::APInt>(Rhs.ValVariant)};
  case EdgeValue::FloatingPoint:
    return {std::get<llvm::APFloat>(Lhs.ValVariant) +
            std::get<llvm::APFloat>(Rhs.ValVariant)};
  case EdgeValue::String:
    return {std::get<std::string>(Lhs.ValVariant) +
            std::get<std::string>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator-(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) -
            std::get<llvm::APInt>(Lhs.ValVariant)};
  case EdgeValue::FloatingPoint:
    // printSemantics(v1.value.asFP) << " <=> ";
    // printSemantics(v2.value.asFP) << std::endl;
    return {std::get<llvm::APFloat>(Lhs.ValVariant) -
            std::get<llvm::APFloat>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator*(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) *
            std::get<llvm::APInt>(Rhs.ValVariant)};
  case EdgeValue::FloatingPoint:
    return {std::get<llvm::APFloat>(Lhs.ValVariant) *
            std::get<llvm::APFloat>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator/(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant)
                .sdiv(std::get<llvm::APInt>(Rhs.ValVariant))};
  case EdgeValue::FloatingPoint:
    return {std::get<llvm::APFloat>(Lhs.ValVariant) /
            std::get<llvm::APFloat>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator%(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant)
                .srem(std::get<llvm::APInt>(Rhs.ValVariant))};
  case EdgeValue::FloatingPoint: {
    llvm::APFloat Fl = std::get<llvm::APFloat>(Lhs.ValVariant);
    Fl.remainder(std::get<llvm::APFloat>(Rhs.ValVariant));
    return {std::move(Fl)};
  }
  default:
    return {nullptr};
  }
}

EdgeValue operator&(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) &
            std::get<llvm::APInt>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator|(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) |
            std::get<llvm::APInt>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator^(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant) ^
            std::get<llvm::APInt>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}

EdgeValue operator<<(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant)
            << std::get<llvm::APInt>(Rhs.ValVariant)};
  default:
    return {nullptr};
  }
}
EdgeValue operator>>(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  if (Lhs.VariantType != Rhs.VariantType) {
    return {nullptr};
  }
  switch (Lhs.VariantType) {
  case EdgeValue::Integer:
    return {std::get<llvm::APInt>(Lhs.ValVariant)
                .ashr(std::get<llvm::APInt>(Rhs.ValVariant))};
  default:
    return {nullptr};
  }
}

// unary operators
EdgeValue EdgeValue::operator-() const {
  if (VariantType == Integer) {
    return {-std::get<llvm::APInt>(ValVariant)};
  }
  return {nullptr};
}

EdgeValue EdgeValue::operator~() const {
  if (VariantType == Integer) {
    return {~std::get<llvm::APInt>(ValVariant)};
  }
  return {nullptr};
}

int EdgeValue::compare(const EdgeValue &Lhs, const EdgeValue &Rhs) {
  switch (Lhs.VariantType) {
  case EdgeValue::Integer: {
    auto Lhsval = std::get<llvm::APInt>(Lhs.ValVariant).getLimitedValue();
    uint64_t Rhsval;
    double RhsvalFp;
    if (Rhs.tryGetInt(Rhsval)) {
      return Lhsval - Rhsval;
    }
    if (Rhs.tryGetFP(RhsvalFp)) {
      return Lhsval < RhsvalFp ? -1 : (Lhsval > RhsvalFp ? 1 : 0);
    }
    break;
  }
  case EdgeValue::FloatingPoint: {
    auto Lhsval = std::get<llvm::APFloat>(Lhs.ValVariant).convertToDouble();
    uint64_t Rhsval;
    double RhsvalFp;
    bool IsInt = Rhs.tryGetInt(Rhsval);
    if (IsInt || Rhs.tryGetFP(RhsvalFp)) {
      if (IsInt) {
        RhsvalFp = Rhsval;
      }

      return Lhsval < RhsvalFp ? -1 : (Lhsval > RhsvalFp ? 1 : 0);
    }

    break;
  }
  case EdgeValue::String: {
    std::string Rhsval;
    if (Rhs.tryGetString(Rhsval)) {
      return std::get<std::string>(Lhs.ValVariant).compare(Rhsval);
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

std::ostream &operator<<(std::ostream &Os, const EdgeValue &Ev) {
  switch (Ev.VariantType) {
  case EdgeValue::Integer: {
    std::string S;
    llvm::raw_string_ostream Ros(S);
    Ros << std::get<llvm::APInt>(Ev.ValVariant);
    return Os << Ros.str();
  }
  case EdgeValue::String:
    return Os << "\"" << std::get<std::string>(Ev.ValVariant) << "\"";
  case EdgeValue::FloatingPoint: {
    return Os << std::get<llvm::APFloat>(Ev.ValVariant).convertToDouble();
  }
  default:
    return Os << "<TOP>";
  }
}

EdgeValue EdgeValue::typecast(Type Dest, unsigned Bits) const {
  switch (Dest) {

  case Integer:
    switch (VariantType) {
    case Integer:
      if (std::get<llvm::APInt>(ValVariant).getBitWidth() <= Bits) {
        return *this;
      }
      return {std::get<llvm::APInt>(ValVariant) & ((1 << Bits) - 1)};
    case FloatingPoint: {
      bool Unused;
      llvm::APSInt Ai;
      std::get<llvm::APFloat>(ValVariant)
          .convertToInteger(Ai, llvm::APFloat::roundingMode::NearestTiesToEven,
                            &Unused);
      return {Ai};
    }
    default:
      return {nullptr};
    }
  case FloatingPoint:
    switch (VariantType) {
    case Integer:
      if (Bits > 32) {
        return {(double)std::get<llvm::APInt>(ValVariant).getSExtValue()};
      }
      return {(float)std::get<llvm::APInt>(ValVariant).getSExtValue()};
    case FloatingPoint:
      return *this;
    default:
      return {nullptr};
    }
  default:
    return {nullptr};
  }
}

EdgeValue EdgeValue::performBinOp(llvm::BinaryOperator::BinaryOps Op,
                                  const EdgeValue &Other) const {
  switch (Op) {
  case llvm::BinaryOperator::BinaryOps::Add:
  case llvm::BinaryOperator::BinaryOps::FAdd:
    return *this + Other;
  case llvm::BinaryOperator::BinaryOps::And:
    return *this & Other;
  case llvm::BinaryOperator::BinaryOps::AShr:
    return *this >> Other;
  case llvm::BinaryOperator::BinaryOps::FDiv:
  case llvm::BinaryOperator::BinaryOps::SDiv:
    return *this / Other;
  case llvm::BinaryOperator::BinaryOps::LShr: {
    if (VariantType != Other.VariantType) {
      return {nullptr};
    }
    switch (VariantType) {
    case EdgeValue::Integer:
      return {std::get<llvm::APInt>(ValVariant)
                  .lshr(std::get<llvm::APInt>(Other.ValVariant))};
    default:
      return {nullptr};
    }
  }
  case llvm::BinaryOperator::BinaryOps::Mul:
  case llvm::BinaryOperator::BinaryOps::FMul:
    return *this * Other;
  case llvm::BinaryOperator::BinaryOps::Or:
    return *this | Other;
  case llvm::BinaryOperator::BinaryOps::Shl:
    return *this << Other;
  case llvm::BinaryOperator::BinaryOps::SRem:
  case llvm::BinaryOperator::BinaryOps::FRem:
    return *this % Other;
  case llvm::BinaryOperator::BinaryOps::Sub:
  case llvm::BinaryOperator::BinaryOps::FSub:
    return *this - Other;
  case llvm::BinaryOperator::BinaryOps::UDiv: {
    if (VariantType != Other.VariantType) {
      return {nullptr};
    }
    switch (VariantType) {
    case EdgeValue::Integer:
      return {std::get<llvm::APInt>(ValVariant)
                  .udiv(std::get<llvm::APInt>(Other.ValVariant))};
    default:
      return {nullptr};
    }
  }
  case llvm::BinaryOperator::BinaryOps::URem: {
    if (VariantType != Other.VariantType) {
      return {nullptr};
    }
    switch (VariantType) {
    case EdgeValue::Integer:
      return {std::get<llvm::APInt>(ValVariant)
                  .urem(std::get<llvm::APInt>(Other.ValVariant))};
    default:
      return {nullptr};
    }
  }
  case llvm::BinaryOperator::BinaryOps::Xor:
    return *this ^ Other;
  default:
    return {nullptr};
  }
}

ev_t performBinOp(llvm::BinaryOperator::BinaryOps Op, const ev_t &Lhs,
                  const ev_t &Rhs, size_t MaxSize) {
  // std::cout << "Perform Binop on " << v1 << " and " << v2 << std::endl;

  if (Lhs.empty() || isTopValue(Lhs) || Rhs.empty() || isTopValue(Rhs)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {{nullptr}};
  }
  ev_t Ret({});
  for (const auto &Ev1 : Lhs) {
    for (const auto &Ev2 : Rhs) {

      Ret.insert(Ev1.performBinOp(Op, Ev2));
      if (Ret.size() > MaxSize) {
        // std::cout << "\t=> <TOP>" << std::endl;
        return ev_t({{nullptr}});
      }
    }
  }
  // std::cout << "\t=> " << ret << std::endl;
  return Ret;
}

ev_t performTypecast(const ev_t &Ev, EdgeValue::Type Dest, unsigned Bits) {
  if (Ev.empty() || isTopValue(Ev)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {{nullptr}};
  }
  ev_t Ret({});
  for (const auto &V : Ev) {
    auto Tc = V.typecast(Dest, Bits);
    if (Tc.isTop()) {
      return ev_t({{nullptr}});
    }
    Ret.insert(Tc);
  }
  return Ret;
}

Ordering compare(const ev_t &Lhs, const ev_t &Rhs) {
  const auto &Smaller = Lhs.size() <= Rhs.size() ? Lhs : Rhs;
  const auto &Larger = Lhs.size() > Rhs.size() ? Lhs : Rhs;

  for (const auto &Elem : Smaller) {
    if (!Larger.count(Elem)) {
      return Ordering::Incomparable;
    }
  }
  return Lhs.size() == Rhs.size()
             ? Ordering::Equal
             : (&Smaller == &Lhs ? Ordering::Less : Ordering::Greater);
}

ev_t join(const ev_t &Lhs, const ev_t &Rhs, size_t MaxSize) {
  // std::cout << "Join " << v1 << " and " << v2 << std::endl;
  if (isTopValue(Lhs) || isTopValue(Rhs)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {{nullptr}};
  }
  ev_t Ret(Lhs.begin(), Lhs.end());

  for (const auto &Elem : Rhs) {
    Ret.insert(Elem);
    if (Ret.size() > MaxSize) {
      // std::cout << "\t=> <TOP>" << std::endl;
      return {{nullptr}};
    }
  }
  // std::cout << "\t=> " << ret << std::endl;

  return Ret;
}

bool isTopValue(const ev_t &V) { return V.size() == 1 && V.begin()->isTop(); }
std::ostream &operator<<(std::ostream &Os, const ev_t &V) {
  Os << "{";
  bool First = true;
  for (const auto &Elem : V) {
    if (First) {
      First = false;
    } else {
      Os << ", ";
    }
    Os << Elem;
  }
  return Os << "}";
}

bool operator<(const ev_t &Lhs, const ev_t &Rhs) {
  if (Lhs.size() >= Rhs.size()) {
    return Lhs != Rhs && (Lhs.empty() || Rhs == ev_t({EdgeValue::TopValue}));
  }
  for (const auto &Elem : Lhs) {
    if (!Rhs.count(Elem)) {
      return false;
    }
  }
  return true;
}

std::string EdgeValue::typeToString(Type Ty) {
  switch (Ty) {
  case Integer:
    return "Integer";
  case FloatingPoint:
    return "FloatingPoint";
  case String:
    return "String";
  default:
    return "Top";
  }
}

} // namespace psr
