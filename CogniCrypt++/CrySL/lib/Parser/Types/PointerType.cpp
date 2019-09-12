#include <Parser/Types/BaseType.h>
#include <Parser/Types/PointerType.h>

namespace CCPP {
namespace Types {

// We decided to not make pointers const
PointerType::PointerType(std::shared_ptr<Type> &underlying)
    : Type(underlying->getName() + "*", false), underlying(underlying) {}

bool PointerType::isPointerType() const { return true; }
const std::shared_ptr<Type> &PointerType::getPointerElementType() const {
  return underlying;
}
bool PointerType::equivalent(const Type *other) const {
  if (other->isPointerType()) {
    auto ty = (PointerType *)other;
    return underlying->equivalent(ty->underlying.get());
  }
  return false;
}
bool PointerType::canBeAssignedTo(const Type *other) const {
  if (other->isPointerType()) {
    auto ty = (PointerType *)other;
    // assign every pointer to void*
    if (ty->underlying->isPrimitiveType()) {
      auto prim = ((BaseType *)ty->underlying.get())->getPrimitiveType();
      if (prim == Type::PrimitiveType::VOID)
        return true;
    }
    // assign nullptr or NULL to every pointer
    else if (underlying->isPrimitiveType()) {
      auto prim = ((BaseType *)underlying.get())->getPrimitiveType();
      if (prim == Type::PrimitiveType::NULL_T)
        return true;
    } else if (underlying->isConstant()) // covariance only with constant types
      return underlying->canBeAssignedTo(ty->underlying.get());
    else
      return underlying->equivalent(ty->underlying.get());
  }
  return false;
}
} // namespace Types
} // namespace CCPP