/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Alexander Meinhold and others
 *****************************************************************************/

#include <cassert>

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"

namespace psr {

std::ostream &printSemantics(const llvm::APFloat &fl) {
  if (&fl.getSemantics() == &llvm::APFloat::IEEEdouble()) {
    return std::cout << "IEEEdouble";
  } else if (&fl.getSemantics() == &llvm::APFloat::IEEEhalf()) {
    return std::cout << "IEEEhalf";
  } else if (&fl.getSemantics() == &llvm::APFloat::IEEEquad()) {
    return std::cout << "IEEEquad";
  } else if (&fl.getSemantics() == &llvm::APFloat::IEEEsingle()) {
    return std::cout << "IEEEsingle";
  } else if (&fl.getSemantics() == &llvm::APFloat::PPCDoubleDouble()) {
    return std::cout << "PPCDoubleDouble";
  } else if (&fl.getSemantics() == &llvm::APFloat::x87DoubleExtended()) {
    return std::cout << "x87DoubleExtended";
  } else if (&fl.getSemantics() == &llvm::APFloat::Bogus()) {
    return std::cout << "Bogus";
  } else {
    return std::cout << "Sth else";
  }
}

const EdgeValue EdgeValue::top = EdgeValue(nullptr);

EdgeValue::EdgeValue(const llvm::Value *val) : type(Top) {
  if (auto cnst = llvm::dyn_cast<llvm::Constant>(val)) {
    if (cnst->getType()->isIntegerTy()) {
      type = Integer;
      value = llvm::APInt(llvm::cast<llvm::ConstantInt>(cnst)->getValue());
    } else if (cnst->getType()->isFloatingPointTy()) {
      type = FloatingPoint;
      auto &cnstFP = llvm::cast<llvm::ConstantFP>(cnst)->getValueAPF();

      llvm::APFloat apf(cnstFP);
      bool unused;
      apf.convert(llvm::APFloat::IEEEdouble(),
                  llvm::APFloat::roundingMode::rmNearestTiesToEven, &unused);
      value = llvm::APFloat(apf);
    } else if (llvm::isa<llvm::ConstantPointerNull>(cnst)) {
      type = String;
      value = std::string();
    } else if (cnst->getType()->isPointerTy() &&
               cnst->getType()->getPointerElementType()->isIntegerTy()) {
      type = String;
      auto gep = llvm::cast<llvm::ConstantExpr>(
          cnst); // already checked, hence cast instead of dyn_cast
      if (auto glob =
              llvm::dyn_cast<llvm::GlobalVariable>(gep->getOperand(0))) {
        value = std::string(
            llvm::cast<llvm::ConstantDataArray>(glob->getInitializer())
                ->getAsCString()
                .str());
      } else {
        // inttoptr
        value = nullptr;
        type = Top;
      }
    } else {
      value = nullptr;
      type = Top;
    }
  } else {
    value = nullptr;
    type = Top;
  }
}

EdgeValue::EdgeValue(const EdgeValue &ev) : type(ev.type) {
  switch (type) {
  case Top:
    value = nullptr;
    break;
  case Integer:
    value = std::get<llvm::APInt>(ev.value);
    break;
  case FloatingPoint:
    value = std::get<llvm::APFloat>(ev.value);
    break;
  case String:
    value = std::get<std::string>(ev.value);
    break;
  }
}

EdgeValue &EdgeValue::operator=(const EdgeValue &ev) {
  this->~EdgeValue();
  new (this) EdgeValue(ev);
  return *this;
}

EdgeValue::~EdgeValue() { value.~variant(); }

EdgeValue::EdgeValue(llvm::APInt &&vi) : type(EdgeValue::Integer) {
  value = llvm::APInt(std::move(vi));
}

EdgeValue::EdgeValue(const llvm::APInt &vi) : type(EdgeValue::Integer) {
  value = llvm::APInt(vi);
}

EdgeValue::EdgeValue(llvm::APFloat &&vf) : type(EdgeValue::FloatingPoint) {
  llvm::APFloat fp = llvm::APFloat(std::move(vf));
  bool unused;
  fp.convert(llvm::APFloat::IEEEdouble(),
             llvm::APFloat::roundingMode::rmNearestTiesToEven, &unused);
  value = fp;
}

EdgeValue::EdgeValue(long long vi) : type(EdgeValue::Integer) {
  value = llvm::APInt(llvm::APInt(sizeof(long long) << 3, vi));
}

EdgeValue::EdgeValue(int vi) : type(EdgeValue::Integer) {
  value = llvm::APInt(llvm::APInt(sizeof(int) << 3, vi));
}

EdgeValue::EdgeValue(double d) : type(EdgeValue::FloatingPoint) {
  value = llvm::APFloat(d);
}

EdgeValue::EdgeValue(float d) : type(EdgeValue::FloatingPoint) {
  value = llvm::APFloat(d);
}

EdgeValue::EdgeValue(std::string &&vs) : type(EdgeValue::String) {
  value = std::string(vs);
}

EdgeValue::EdgeValue(std::nullptr_t) : type(EdgeValue::Top) {}
bool EdgeValue::tryGetInt(uint64_t &res) const {
  if (type != Integer)
    return false;
  res = std::get<llvm::APInt>(value).getLimitedValue();
  return true;
}

bool EdgeValue::tryGetFP(double &res) const {
  if (type != FloatingPoint)
    return false;
  res = std::get<llvm::APFloat>(value).convertToDouble();
  return true;
}

bool EdgeValue::tryGetString(std::string &res) const {
  if (type != String)
    return false;
  res = std::get<std::string>(value);
  return true;
}

bool EdgeValue::isTop() const { return type == Top; }

bool EdgeValue::isNumeric() const {
  return type == Integer || type == FloatingPoint;
}

bool EdgeValue::isString() const { return type == String; }

EdgeValue::Type EdgeValue::getKind() const { return type; }

EdgeValue::operator bool() {
  switch (type) {
  case Integer:
    return !std::get<llvm::APInt>(value).isNullValue();
  case FloatingPoint:
    return std::get<llvm::APFloat>(value).isNonZero();
  case String:
    return !std::get<std::string>(value).empty();
  default:
    break;
  }
  return false;
}

bool operator==(const EdgeValue &v1, const EdgeValue &v2) {
  // std::cout << "Compare edge values" << std::endl;
  if (v1.type != v2.type) {
    // std::cout << "Comparing incompatible types" << std::endl;
    return false;
  }
  switch (v1.type) {
  case EdgeValue::Top:
    return true;
  case EdgeValue::Integer:
    // if (v1.value.asInt != v2.value.asInt)
    //  std::cout << "integer unequal" << std::endl;
    return std::get<llvm::APInt>(v1.value) == std::get<llvm::APInt>(v2.value);
  case EdgeValue::FloatingPoint: {
    // std::cout << "compare floating points" << std::endl;
    auto cp = std::get<llvm::APFloat>(v1.value).compare(
        std::get<llvm::APFloat>(v2.value));
    if (cp == llvm::APFloat::cmpResult::cmpEqual) {
      // std::cout << "FP equal" << std::endl;
      return true;
    }
    auto d1 = std::get<llvm::APFloat>(v1.value).convertToDouble();
    auto d2 = std::get<llvm::APFloat>(v2.value).convertToDouble();

    const double epsilon = 0.000001;
    // std::cout << "Compare " << d1 << " against " << d2 << std::endl;
    return d1 == d2 || d1 - d2 < epsilon || d2 - d1 < epsilon;
  }
  case EdgeValue::String:
    // if (v1.value.asString != v2.value.asString)
    //  std::cout << "String unequal" << std::endl;
    return std::get<std::string>(v1.value) == std::get<std::string>(v2.value);
  default: // will not happen
    std::cerr << "FATAL ERROR" << std::endl;
    return false;
  }
}

bool EdgeValue::sqSubsetEq(const EdgeValue &other) const {
  return other.isTop() || other.type == type;
}

// binary operators
EdgeValue operator+(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) +
                     std::get<llvm::APInt>(v2.value));
  case EdgeValue::FloatingPoint:
    return EdgeValue(std::get<llvm::APFloat>(v1.value) +
                     std::get<llvm::APFloat>(v2.value));
  case EdgeValue::String:
    return EdgeValue(std::get<std::string>(v1.value) +
                     std::get<std::string>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator-(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) -
                     std::get<llvm::APInt>(v1.value));
  case EdgeValue::FloatingPoint:
    // printSemantics(v1.value.asFP) << " <=> ";
    // printSemantics(v2.value.asFP) << std::endl;
    return EdgeValue(std::get<llvm::APFloat>(v1.value) -
                     std::get<llvm::APFloat>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator*(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) *
                     std::get<llvm::APInt>(v2.value));
  case EdgeValue::FloatingPoint:
    return EdgeValue(std::get<llvm::APFloat>(v1.value) *
                     std::get<llvm::APFloat>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator/(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(
        std::get<llvm::APInt>(v1.value).sdiv(std::get<llvm::APInt>(v2.value)));
  case EdgeValue::FloatingPoint:
    return EdgeValue(std::get<llvm::APFloat>(v1.value) /
                     std::get<llvm::APFloat>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator%(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(
        std::get<llvm::APInt>(v1.value).srem(std::get<llvm::APInt>(v2.value)));
  case EdgeValue::FloatingPoint: {
    llvm::APFloat fl = std::get<llvm::APFloat>(v1.value);
    fl.remainder(std::get<llvm::APFloat>(v2.value));
    return EdgeValue(std::move(fl));
  }
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator&(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) &
                     std::get<llvm::APInt>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator|(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) |
                     std::get<llvm::APInt>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator^(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value) ^
                     std::get<llvm::APInt>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator<<(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(std::get<llvm::APInt>(v1.value)
                     << std::get<llvm::APInt>(v2.value));
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator>>(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(
        std::get<llvm::APInt>(v1.value).ashr(std::get<llvm::APInt>(v2.value)));
  default:
    return EdgeValue(nullptr);
  }
}

// unary operators
EdgeValue EdgeValue::operator-() const {
  if (type == Integer)
    return EdgeValue(-std::get<llvm::APInt>(value));
  return EdgeValue(nullptr);
}

EdgeValue EdgeValue::operator~() const {
  if (type == Integer)
    return EdgeValue(~std::get<llvm::APInt>(value));
  return EdgeValue(nullptr);
}

int EdgeValue::compare(const EdgeValue &v1, const EdgeValue &v2) {
  switch (v1.type) {
  case EdgeValue::Integer: {
    auto v1_val = std::get<llvm::APInt>(v1.value).getLimitedValue();
    uint64_t v2_val;
    double v2_val_fp;
    if (v2.tryGetInt(v2_val)) {
      return v1_val - v2_val;
    } else if (v2.tryGetFP(v2_val_fp)) {
      return v1_val < v2_val_fp ? -1 : (v1_val > v2_val_fp ? 1 : 0);
    }
    break;
  }
  case EdgeValue::FloatingPoint: {
    auto v1_val = std::get<llvm::APFloat>(v1.value).convertToDouble();
    uint64_t v2_val;
    double v2_val_fp;
    bool isInt = v2.tryGetInt(v2_val);
    if (isInt || v2.tryGetFP(v2_val_fp)) {
      if (isInt)
        v2_val_fp = v2_val;

      return v1_val < v2_val_fp ? -1 : (v1_val > v2_val_fp ? 1 : 0);
    }

    break;
  }
  case EdgeValue::String: {
    std::string v2_val;
    if (v2.tryGetString(v2_val)) {
      return std::get<std::string>(v1.value).compare(v2_val);
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

std::ostream &operator<<(std::ostream &os, const EdgeValue &ev) {
  switch (ev.type) {
  case EdgeValue::Integer: {
    std::string s;
    llvm::raw_string_ostream ros(s);
    ros << std::get<llvm::APInt>(ev.value);
    return os << ros.str();
  }
  case EdgeValue::String:
    return os << "\"" << std::get<std::string>(ev.value) << "\"";
  case EdgeValue::FloatingPoint: {
    return os << std::get<llvm::APFloat>(ev.value).convertToDouble();
  }
  default:
    return os << "<TOP>";
  }
}

EdgeValue EdgeValue::typecast(Type dest, unsigned bits) const {
  switch (dest) {

  case Integer:
    switch (type) {
    case Integer:
      if (std::get<llvm::APInt>(value).getBitWidth() <= bits)
        return *this;
      else
        return EdgeValue(std::get<llvm::APInt>(value) & ((1 << bits) - 1));
    case FloatingPoint: {
      bool unused;
      llvm::APSInt ai;
      std::get<llvm::APFloat>(value).convertToInteger(
          ai, llvm::APFloat::roundingMode::rmNearestTiesToEven, &unused);
      return EdgeValue(ai);
    }
    default:
      return EdgeValue(nullptr);
    }
  case FloatingPoint:
    switch (type) {
    case Integer:
      if (bits > 32)
        return EdgeValue((double)std::get<llvm::APInt>(value).getSExtValue());
      else
        return EdgeValue((float)std::get<llvm::APInt>(value).getSExtValue());
    case FloatingPoint:
      return *this;
    default:
      return EdgeValue(nullptr);
    }
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue EdgeValue::performBinOp(llvm::BinaryOperator::BinaryOps op,
                                  const EdgeValue &other) const {
  switch (op) {
  case llvm::BinaryOperator::BinaryOps::Add:
  case llvm::BinaryOperator::BinaryOps::FAdd:
    return *this + other;
  case llvm::BinaryOperator::BinaryOps::And:
    return *this & other;
  case llvm::BinaryOperator::BinaryOps::AShr:
    return *this >> other;
  case llvm::BinaryOperator::BinaryOps::FDiv:
  case llvm::BinaryOperator::BinaryOps::SDiv:
    return *this / other;
  case llvm::BinaryOperator::BinaryOps::LShr: {
    if (type != other.type)
      return EdgeValue(nullptr);
    switch (type) {
    case EdgeValue::Integer:
      return EdgeValue(std::get<llvm::APInt>(value).lshr(
          std::get<llvm::APInt>(other.value)));
    default:
      return EdgeValue(nullptr);
    }
  }
  case llvm::BinaryOperator::BinaryOps::Mul:
  case llvm::BinaryOperator::BinaryOps::FMul:
    return *this * other;
  case llvm::BinaryOperator::BinaryOps::Or:
    return *this | other;
  case llvm::BinaryOperator::BinaryOps::Shl:
    return *this << other;
  case llvm::BinaryOperator::BinaryOps::SRem:
  case llvm::BinaryOperator::BinaryOps::FRem:
    return *this % other;
  case llvm::BinaryOperator::BinaryOps::Sub:
  case llvm::BinaryOperator::BinaryOps::FSub:
    return *this - other;
  case llvm::BinaryOperator::BinaryOps::UDiv: {
    if (type != other.type)
      return EdgeValue(nullptr);
    switch (type) {
    case EdgeValue::Integer:
      return EdgeValue(std::get<llvm::APInt>(value).udiv(
          std::get<llvm::APInt>(other.value)));
    default:
      return EdgeValue(nullptr);
    }
  }
  case llvm::BinaryOperator::BinaryOps::URem: {
    if (type != other.type)
      return EdgeValue(nullptr);
    switch (type) {
    case EdgeValue::Integer:
      return EdgeValue(std::get<llvm::APInt>(value).urem(
          std::get<llvm::APInt>(other.value)));
    default:
      return EdgeValue(nullptr);
    }
  }
  case llvm::BinaryOperator::BinaryOps::Xor:
    return *this ^ other;
  default:
    return EdgeValue(nullptr);
  }
}

ev_t performBinOp(llvm::BinaryOperator::BinaryOps op, const ev_t &v1,
                  const ev_t &v2, size_t maxSize) {
  // std::cout << "Perform Binop on " << v1 << " and " << v2 << std::endl;

  if (v1.empty() || isTopValue(v1) || v2.empty() || isTopValue(v2)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {EdgeValue(nullptr)};
  }
  ev_t ret({});
  for (auto &ev1 : v1) {
    for (auto &ev2 : v2) {

      ret.insert(ev1.performBinOp(op, ev2));
      if (ret.size() > maxSize) {
        // std::cout << "\t=> <TOP>" << std::endl;
        return ev_t({EdgeValue(nullptr)});
      }
    }
  }
  // std::cout << "\t=> " << ret << std::endl;
  return ret;
}

ev_t performTypecast(const ev_t &ev, EdgeValue::Type dest, unsigned bits) {
  if (ev.empty() || isTopValue(ev)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {EdgeValue(nullptr)};
  }
  ev_t ret({});
  for (auto &v : ev) {
    auto tc = v.typecast(dest, bits);
    if (tc.isTop())
      return ev_t({EdgeValue(nullptr)});
    ret.insert(tc);
  }
  return ret;
}

Ordering compare(const ev_t &v1, const ev_t &v2) {
  auto &smaller = v1.size() <= v2.size() ? v1 : v2;
  auto &larger = v1.size() > v2.size() ? v1 : v2;

  for (auto &elem : smaller) {
    if (!larger.count(elem)) {
      return Ordering::Incomparable;
    }
  }
  return v1.size() == v2.size()
             ? Ordering::Equal
             : (&smaller == &v1 ? Ordering::Less : Ordering::Greater);
}

ev_t join(const ev_t &v1, const ev_t &v2, size_t maxSize) {
  // std::cout << "Join " << v1 << " and " << v2 << std::endl;
  if (isTopValue(v1) || isTopValue(v2)) {
    // std::cout << "\t=> <TOP>" << std::endl;
    return {EdgeValue(nullptr)};
  }
  ev_t ret(v1.begin(), v1.end());

  for (auto &elem : v2) {
    ret.insert(elem);
    if (ret.size() > maxSize) {
      // std::cout << "\t=> <TOP>" << std::endl;
      return {EdgeValue(nullptr)};
    }
  }
  // std::cout << "\t=> " << ret << std::endl;

  return ret;
}

bool isTopValue(const ev_t &v) { return v.size() == 1 && v.begin()->isTop(); }
std::ostream &operator<<(std::ostream &os, const ev_t &v) {
  os << "{";
  bool frst = true;
  for (auto &elem : v) {
    if (frst)
      frst = false;
    else
      os << ", ";
    os << elem;
  }
  return os << "}";
}

bool operator<(const ev_t &v1, const ev_t &v2) {
  if (v1.size() >= v2.size()) {
    return v1 != v2 && (v1.empty() || v2 == ev_t({EdgeValue::top}));
  } else {
    for (auto &elem : v1) {
      if (!v2.count(elem))
        return false;
    }
    return true;
  }
}

std::string EdgeValue::typeToString(Type ty) {
  switch (ty) {
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
