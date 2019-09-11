#pragma once
#include <memory>
#include <string>

namespace CCPP {
namespace Types {
/// \brief The root of the type hierarchy
///
/// Compare types using the Type::equivalent(Type*) method
class Type : public std::enable_shared_from_this<Type> {
  const std::string Name;
  const bool isConst;

protected:
  Type(const std::string &name, bool isConst) : Name(name), isConst(isConst) {}

public:
  virtual ~Type() = default;
  /// \brief Kinds of primitive types
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
    NULL_T,
    TOP,
    BOT
  };
  /// \brief A std::shared_ptr<const Type> holding the this pointer
  const std::shared_ptr<const Type> getShared() const {
    return shared_from_this();
  }
  /// \brief The (qualified) name of the type
  const std::string &getName() const { return Name; }
  /// \brief A weak form of a subtype relationship
  ///
  /// Does not take real subtype-relationships into account;
  /// only converts between primitives
  virtual bool canBeAssignedTo(const Type *other) const = 0;
  /// \brief Type equivalence
  virtual bool equivalent(const Type *other) const { return this == other; }
  /// \brief True, iff this type is a pointer-type
  virtual bool isPointerType() const { return false; }
  /// \brief True, iff this type is a C-style array-type
  virtual bool isArrayType() const { return false; };
  /// \brief True, iff this type is a primitive type (integer, boolean, or
  /// floating-point)
  virtual bool isPrimitiveType() const { return false; }
  /// \brief Gets the Type::PrimitiveType, which indicates the kind of primitive
  /// type this type is. PrimitiveType::NONE, when this is not primitive.
  virtual PrimitiveType getPrimitiveType() const { return PrimitiveType::NONE; }
  /// \brief Creates a join of this type and the specified other type using
  /// Type::canBeAssignedTo(Type*).
  ///
  /// Note, that it has the same constraints as Type::canBeAssignedTo(Type*).
  virtual std::shared_ptr<const Type>
  join(const std::shared_ptr<const Type> &other) const {
    return other && this->equivalent(other.get()) ? other : nullptr;
  }
  /// \brief True, iff the 'const' modifier is used for this type
  bool isConstant() const { return isConst; }
};
} // namespace Types
} // namespace CCPP