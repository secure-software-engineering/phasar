#include <Types/BaseType.h>
#include <unordered_map>
namespace CCPP {
namespace Types {
using namespace std;

BaseType::BaseType(const string &name, bool isConst)
    : Type(name, isConst), prim(PrimitiveType::NONE) {}
BaseType::BaseType(const string &name, Type::PrimitiveType prim)
    : Type(name, true), prim(prim) {}

bool CCPP::Types::BaseType::isPrimitiveType() const {
  return prim != PrimitiveType::NONE;
}
Type::PrimitiveType BaseType::getPrimitiveType() const { return prim; }
bool BaseType::equivalent(Type *other) const {
  if (other->isPrimitiveType()) {
    auto otherPrim = ((BaseType *)other)->getPrimitiveType();
    return otherPrim == prim;
  }
  return getName() == other->getName();
}

bool BaseType::canBeAssignedTo(Type *other) const {
  if (other->isPrimitiveType()) {
    // TODO be more precise here
    return prim <= ((BaseType *)other)->getPrimitiveType();
  } else {
    return equivalent(other);
  }
}

shared_ptr<const Type>
BaseType::join(const shared_ptr<const Type> &other) const {
  if (prim == PrimitiveType::NONE || !other->isPrimitiveType()) {
    // TODO: report proper error value
    return this->Type::join(other);
  }
  auto otherPrim = ((BaseType *)other.get())->getPrimitiveType();
  // TODO: be more precise here
  return prim >= otherPrim ? getShared() : other;
}

} // namespace Types
} // namespace CCPP