#include "PointerType.h"
#include "BaseType.h"
namespace CCPP {
namespace Types {

// We decided to not make pointers const
PointerType::PointerType(Type *underlying)
    : Type(underlying->getName() + "*", false), underlying(underlying) {}

virtual bool PointerType::isPointerType() const override { return true; }
Type *PointerType::getPointerElementType() const { return underlying; }
virtual bool PointerType::equivalent(Type *other) const override {
  if (other->isPointerType()) {
    auto ty = (PointerType *)other;
    return underlying->equivalent(ty->underlying);
  }
  return false;
}
virtual bool PointerType::canBeAssignedTo(Type *other) const override {
  if (other->isPointerType()) {
    auto ty = (PointerType *)other;
    // assign every pointer to void*
    if (ty->underlying->isPrimitiveType()) {
      auto prim = ((BaseType *)ty->underlying)->getPrimitiveType();
      if (prim == Type::PrimitiveType::VOID)
        return true;
    }
    // assign nullptr or NULL to every pointer
    else if (underlying->isPrimitiveType()) {
      auto prim = ((BaseType *)underlying)->getPrimitiveType();
      if (prim == Type::PrimitiveType::NULL_T)
        return true;
    } else if (underlying->isConstant()) // covariance only with constant types
      return underlying->canBeAssignedTo(ty->underlying);
    else
      return underlying->equivalent(ty->underlying);
  }
  return false;
}
} // namespace Types
} // namespace CCPP