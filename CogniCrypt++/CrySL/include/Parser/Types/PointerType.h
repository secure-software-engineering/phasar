#pragma once
#include "Type.h"
#include <memory>
namespace CCPP {
namespace Types {
/// \brief A type representing arbitrary pointers
class PointerType : public Type {
  std::shared_ptr<Type> underlying;

public:
  PointerType(std::shared_ptr<Type> &underlying);
  virtual bool isPointerType() const override;
  /// \brief The type of the values to which a pointer of this type may point to
  const std::shared_ptr<Type> &getPointerElementType() const;
  virtual bool equivalent(const Type *other) const override;
  virtual bool canBeAssignedTo(const Type *other) const override;
};
} // namespace Types
} // namespace CCPP
