#pragma once
#include <string>

namespace CCPP {
namespace Types {
/// \brief The root of the type hierarchy
class Type {
  const std::string Name;
  const bool isConst;

protected:
  Type(const std::string &name, bool isConst) : Name(name), isConst(isConst) {}

public:
  enum PrimitiveType {
    NONE,
    BOOL,
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    LONGLONG,
    ULONGLONG,
    FLOAT,
    DOUBLE,
    LONGDOUBLE,
    SIZE_T,
    VOID,
    NULL_T
  };
  const std::string &getName() const { return Name; }
  /// \brief a weak for of a subtype relationship
  virtual bool canBeAssignedTo(Type *other) const = 0;
  /// \brief Type equivalence
  virtual bool equivalent(Type *other) const { return this == other; }
  virtual bool isPointerType() const { return false; }
  virtual bool isArrayType() const { return false; };
  virtual bool isPrimitiveType() const { return false; };
  bool isConstant() const { return isConst; }
};
} // namespace Types
} // namespace CCPP