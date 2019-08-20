#pragma once
#include <memory>
#include <string>

namespace CCPP {
namespace Types {
/// \brief The root of the type hierarchy
class Type : public std::enable_shared_from_this<Type> {
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
    SIZE_T,
    LONGLONG,
    ULONGLONG,
    FLOAT,
    DOUBLE,
    LONGDOUBLE,
    VOID,
    NULL_T
  };
  const std::shared_ptr<const Type> getShared() const {
    return shared_from_this();
  }
  const std::string &getName() const { return Name; }
  /// \brief a weak for of a subtype relationship
  virtual bool canBeAssignedTo(Type *other) const = 0;
  /// \brief Type equivalence
  virtual bool equivalent(Type *other) const { return this == other; }
  virtual bool isPointerType() const { return false; }
  virtual bool isArrayType() const { return false; };
  virtual bool isPrimitiveType() const { return false; }
  virtual PrimitiveType getPrimitiveType() const { return PrimitiveType::NONE; }
  virtual std::shared_ptr<const Type>
  join(const std::shared_ptr<const Type> &other) const {
    return shared_from_this();
  }
  bool isConstant() const { return isConst; }
};
} // namespace Types
} // namespace CCPP