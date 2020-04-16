#pragma once
#include <iostream>
#include <memory>
#include <unordered_set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/Ordering.h"

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"

namespace CCPP::LCUtils {
class EdgeValue {
public:
  enum Type { Top, Integer, String, FloatingPoint };
  union Value {
    llvm::APInt asInt;
    llvm::APFloat asFP;
    std::string asString;
    std::nullptr_t asTop;

    Value() { asTop = nullptr; }
    ~Value() {}
  };

private:
  Type type;
  Value value;

public:
  EdgeValue(const llvm::Value *val);
  EdgeValue(const EdgeValue &ev);
  EdgeValue(llvm::APInt &&vi);
  EdgeValue(const llvm::APInt &vi);
  EdgeValue(llvm::APFloat &&vf);
  EdgeValue(long long vi);
  EdgeValue(int vi);
  EdgeValue(double d);
  EdgeValue(float d);
  EdgeValue(std::string &&vs);
  EdgeValue(std::nullptr_t);
  ~EdgeValue();
  const static EdgeValue top;
  bool tryGetInt(uint64_t &res) const;
  bool tryGetFP(double &res) const;
  bool tryGetString(std::string &res) const;
  bool isTop() const;
  bool isNumeric() const;
  bool isString() const;
  Type getKind() const;
  // std::unique_ptr<ObjectLLVM> asObjLLVM(llvm::LLVMContext &ctx) const;
  bool sqSubsetEq(const EdgeValue &other) const;
  EdgeValue performBinOp(llvm::BinaryOperator::BinaryOps op,
                         const EdgeValue &other) const;
  EdgeValue typecast(Type dest, unsigned bits) const;
  EdgeValue &operator=(const EdgeValue &ev);

  operator bool();
  friend bool operator==(const EdgeValue &v1, const EdgeValue &v2);

  // binary operators
  friend EdgeValue operator+(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator-(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator*(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator/(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator%(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator&(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator|(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator^(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator<<(const EdgeValue &v1, const EdgeValue &v2);
  friend EdgeValue operator>>(const EdgeValue &v1, const EdgeValue &v2);
  static int compare(const EdgeValue &v1, const EdgeValue &v2);

  // unary operators
  EdgeValue operator-() const;
  EdgeValue operator~() const;
  friend std::ostream &operator<<(std::ostream &os, const EdgeValue &ev);
  static std::string typeToString(Type ty);
};
class EdgeValueSet;
typedef EdgeValueSet ev_t;

ev_t performBinOp(llvm::BinaryOperator::BinaryOps op, const ev_t &v1,
                  const ev_t &v2, size_t maxSize);
ev_t performTypecast(const ev_t &ev, EdgeValue::Type dest, unsigned bits);
Ordering compare(const ev_t &v1, const ev_t &v2);
ev_t join(const ev_t &v1, const ev_t &v2, size_t maxSize);
/// \brief implements square subset equal
bool operator<(const ev_t &v1, const ev_t &v2);
bool isTopValue(const ev_t &v);
std::ostream &operator<<(std::ostream &os, const ev_t &v);
} // namespace CCPP::LCUtils
namespace std {
template <> struct hash<CCPP::LCUtils::EdgeValue> {
  hash() {}
  size_t operator()(const CCPP::LCUtils::EdgeValue &val) const {
    auto hc = hash<int>()(val.getKind());
    uint64_t asInt;
    double asFloat;
    string asString;
    if (val.tryGetInt(asInt))
      return hash<uint64_t>()(asInt) * 31 + hc;
    else if (val.tryGetFP(asFloat))
      return hash<double>()(round(asFloat)) * 31 + hc;
    else if (val.tryGetString(asString))
      return hash<string>()(asString) * 31 + hc;
    return hc;
  }
};
} // namespace std