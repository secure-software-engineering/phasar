#include <Types/ArrayType.h>
namespace CCPP {
namespace Types {

ArrayType::ArrayType(std::shared_ptr<Type> &underlying, long long length)
    : PointerType(underlying), length(length) {}
long long ArrayType::getArrayLength() const { return length; }
virtual bool ArrayType::isArrayType() const override { return true; }
virtual bool ArrayType::equivalent(Type *other) const override {
  if (other->isArrayType() && this->PointerType::equivalent(other)) {
    return length == ((ArrayType *)other)->getArrayLength();
  }
  return false;
}

} // namespace Types
} // namespace CCPP
