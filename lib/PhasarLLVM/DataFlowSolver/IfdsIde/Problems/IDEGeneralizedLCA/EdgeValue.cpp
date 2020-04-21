#define BOOST_STACKTRACE_USE_ADDR2LINE

#include <cassert>

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

#include <boost/stacktrace.hpp>

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
  memset(&value, 0, sizeof(value));
  if (auto cnst = llvm::dyn_cast<llvm::Constant>(val)) {

    if (cnst->getType()->isIntegerTy()) {
      type = Integer;
      new (&value.asInt)
          llvm::APInt(llvm::cast<llvm::ConstantInt>(cnst)->getValue());
    } else if (cnst->getType()->isFloatingPointTy()) {
      type = FloatingPoint;
      auto &cnstFP = llvm::cast<llvm::ConstantFP>(cnst)->getValueAPF();

      llvm::APFloat apf(cnstFP);
      bool unused;
      apf.convert(llvm::APFloat::IEEEdouble(),
                  llvm::APFloat::roundingMode::rmNearestTiesToEven, &unused);

      new (&value.asFP) llvm::APFloat(apf);
    } else if (llvm::isa<llvm::ConstantPointerNull>(cnst)) {
      type = String;
      new (&value.asString) std::string();
    } else if (cnst->getType()->isPointerTy() &&
               cnst->getType()->getPointerElementType()->isIntegerTy()) {
      type = String;
      auto gep = llvm::cast<llvm::ConstantExpr>(
          cnst); // already checked, hence cast instead of dyn_cast

      if (auto glob =
              llvm::dyn_cast<llvm::GlobalVariable>(gep->getOperand(0))) {

        new (&value.asString) std::string(
            llvm::cast<llvm::ConstantDataArray>(glob->getInitializer())
                ->getAsCString()
                .str());
      } else {
        // inttoptr
        value.asTop = nullptr;
        type = Top;
      }
    } else {
      value.asTop = nullptr;
      type = Top;
    }
  } else {
    value.asTop = nullptr;
    type = Top;
  }
}

EdgeValue::EdgeValue(const EdgeValue &ev) : type(ev.type) {
  switch (type) {
  case Top:
    value.asTop = nullptr;
    break;
  case Integer:
    new (&value.asInt) llvm::APInt(ev.value.asInt);
    break;
  case FloatingPoint:
    // value.asFP = ev.value.asFP;
    new (&value.asFP) llvm::APFloat(ev.value.asFP);
    break;
  case String:
    new (&value.asString) std::string(ev.value.asString);
    break;
  }
}

EdgeValue &EdgeValue::operator=(const EdgeValue &ev) {
  this->~EdgeValue();
  new (this) EdgeValue(ev);
  return *this;
}

EdgeValue::~EdgeValue() {
  switch (type) {
  case Integer:
    value.asInt.~APInt();
    break;
  case FloatingPoint:
    value.asFP.~APFloat();
    break;
  case String:
    value.asString.~basic_string();
    break;
  default:
    break;
  }
}

EdgeValue::EdgeValue(llvm::APInt &&vi) : type(EdgeValue::Integer) {
  new (&value.asInt) llvm::APInt(std::move(vi));
}

EdgeValue::EdgeValue(const llvm::APInt &vi) : type(EdgeValue::Integer) {
  new (&value.asInt) llvm::APInt(vi);
}

EdgeValue::EdgeValue(llvm::APFloat &&vf) : type(EdgeValue::FloatingPoint) {
  // value.asFP = vf;
  new (&value.asFP) llvm::APFloat(std::move(vf));
  bool unused;
  value.asFP.convert(llvm::APFloat::IEEEdouble(),
                     llvm::APFloat::roundingMode::rmNearestTiesToEven, &unused);
}

EdgeValue::EdgeValue(long long vi) : type(EdgeValue::Integer) {
  new (&value.asInt) llvm::APInt(llvm::APInt(sizeof(long long) << 3, vi));
}

EdgeValue::EdgeValue(int vi) : type(EdgeValue::Integer) {
  new (&value.asInt) llvm::APInt(llvm::APInt(sizeof(int) << 3, vi));
}

EdgeValue::EdgeValue(double d) : type(EdgeValue::FloatingPoint) {
  // value.asFP = llvm::APFloat(d);
  new (&value.asFP) llvm::APFloat(d);
}

EdgeValue::EdgeValue(float d) : type(EdgeValue::FloatingPoint) {
  // value.asFP = llvm::APFloat(d);
  new (&value.asFP) llvm::APFloat(d);
}

EdgeValue::EdgeValue(std::string &&vs) : type(EdgeValue::String) {
  new (&value.asString) std::string(vs);
}

EdgeValue::EdgeValue(std::nullptr_t) : type(EdgeValue::Top) {}
bool EdgeValue::tryGetInt(uint64_t &res) const {
  if (type != Integer)
    return false;
  res = value.asInt.getLimitedValue();
  return true;
}

bool EdgeValue::tryGetFP(double &res) const {
  if (type != FloatingPoint)
    return false;
  res = value.asFP.convertToDouble();
  return true;
}

bool EdgeValue::tryGetString(std::string &res) const {
  if (type != String)
    return false;
  res = value.asString;
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
    return !value.asInt.isNullValue();
  case FloatingPoint:
    return value.asFP.isNonZero();
  case String:
    return !value.asString.empty();

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
    return v1.value.asInt == v2.value.asInt;
  case EdgeValue::FloatingPoint: {
    // std::cout << "compare floating points" << std::endl;
    auto cp = v1.value.asFP.compare(v2.value.asFP);
    if (cp == llvm::APFloat::cmpResult::cmpEqual) {
      // std::cout << "FP equal" << std::endl;
      return true;
    }
    auto d1 = v1.value.asFP.convertToDouble();
    auto d2 = v2.value.asFP.convertToDouble();
    const double epsilon = 0.000001;
    // std::cout << "Compare " << d1 << " against " << d2 << std::endl;
    return d1 == d2 || d1 - d2 < epsilon || d2 - d1 < epsilon;
  }
  case EdgeValue::String:
    // if (v1.value.asString != v2.value.asString)
    //  std::cout << "String unequal" << std::endl;
    return v1.value.asString == v2.value.asString;
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
    return EdgeValue(v1.value.asInt + v2.value.asInt);
  case EdgeValue::FloatingPoint:
    return EdgeValue(v1.value.asFP + v2.value.asFP);
  case EdgeValue::String:
    return EdgeValue(v1.value.asString + v2.value.asString);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator-(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt - v2.value.asInt);
  case EdgeValue::FloatingPoint:
    // printSemantics(v1.value.asFP) << " <=> ";
    // printSemantics(v2.value.asFP) << std::endl;
    return EdgeValue(v1.value.asFP - v2.value.asFP);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator*(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt * v2.value.asInt);
  case EdgeValue::FloatingPoint:
    return EdgeValue(v1.value.asFP * v2.value.asFP);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator/(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt.sdiv(v2.value.asInt));
  case EdgeValue::FloatingPoint:
    return EdgeValue(v1.value.asFP / v2.value.asFP);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator%(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt.srem(v2.value.asInt));
  case EdgeValue::FloatingPoint: {
    llvm::APFloat fl = v1.value.asFP;
    fl.remainder(v2.value.asFP);
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
    return EdgeValue(v1.value.asInt & v2.value.asInt);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator|(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt | v2.value.asInt);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator^(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt ^ v2.value.asInt);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator<<(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt << v2.value.asInt);
  default:
    return EdgeValue(nullptr);
  }
}

EdgeValue operator>>(const EdgeValue &v1, const EdgeValue &v2) {
  if (v1.type != v2.type)
    return EdgeValue(nullptr);
  switch (v1.type) {
  case EdgeValue::Integer:
    return EdgeValue(v1.value.asInt.ashr(v2.value.asInt));
  default:
    return EdgeValue(nullptr);
  }
}

// unary operators
EdgeValue EdgeValue::operator-() const {
  if (type == Integer)
    return EdgeValue(-value.asInt);
  return EdgeValue(nullptr);
}

EdgeValue EdgeValue::operator~() const {
  if (type == Integer)
    return EdgeValue(~value.asInt);
  return EdgeValue(nullptr);
}

int EdgeValue::compare(const EdgeValue &v1, const EdgeValue &v2) {
  switch (v1.type) {
  case EdgeValue::Integer: {
    auto v1_val = v1.value.asInt.getLimitedValue();
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
    auto v1_val = v1.value.asFP.convertToDouble();
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
      return v1.value.asString.compare(v2_val);
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
    ros << ev.value.asInt;
    return os << ros.str();
  }
  case EdgeValue::String:
    return os << "\"" << ev.value.asString << "\"";
  case EdgeValue::FloatingPoint: {
    return os << ev.value.asFP.convertToDouble();
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
      if (value.asInt.getBitWidth() <= bits)
        return *this;
      else
        return EdgeValue(value.asInt & ((1 << bits) - 1));
    case FloatingPoint: {
      bool unused;
      llvm::APSInt ai;
      value.asFP.convertToInteger(
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
        return EdgeValue((double)value.asInt.getSExtValue());
      else
        return EdgeValue((float)value.asInt.getSExtValue());
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
      return EdgeValue(value.asInt.lshr(other.value.asInt));
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
      return EdgeValue(value.asInt.udiv(other.value.asInt));
    default:
      return EdgeValue(nullptr);
    }
  }
  case llvm::BinaryOperator::BinaryOps::URem: {
    if (type != other.type)
      return EdgeValue(nullptr);
    switch (type) {
    case EdgeValue::Integer:
      return EdgeValue(value.asInt.urem(other.value.asInt));
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
