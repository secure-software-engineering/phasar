#pragma once
#include "Type.h"
#include <memory>
namespace CCPP {
namespace Types {
class PointerType : public Type {
  std::shared_ptr<Type> underlying;

public:
  PointerType(std::shared_ptr<Type> &underlying);
  virtual bool isPointerType() const override;
  std::shared_ptr<Type> &getPointerElementType() const;
  virtual bool equivalent(Type *other) const override;
  virtual bool canBeAssignedTo(Type *other) const override;
};
} // namespace Types
} // namespace CCPP
