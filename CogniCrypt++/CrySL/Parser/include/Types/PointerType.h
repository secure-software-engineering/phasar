#pragma once
#include "Type.h"
namespace CCPP {
namespace Types {
class PointerType : public Type {
  Type *underlying;

public:
  PointerType(Type *underlying);
  virtual bool isPointerType() const override;
  Type *getPointerElementType() const;
  virtual bool equivalent(Type *other) const override;
  virtual bool canBeAssignedTo(Type *other) const override;
};
} // namespace Types
} // namespace CCPP
