#include <Parser/Types/ArrayType.h>
namespace CCPP {
namespace Types {

ArrayType::ArrayType(std::shared_ptr<Type> &underlying, long long length)
    : PointerType(underlying), length(length) {}
long long ArrayType::getArrayLength() const { return length; }
bool ArrayType::isArrayType() const { return true; }
bool ArrayType::equivalent(const Type *other) const {
  if (other->isArrayType() && this->PointerType::equivalent(other)) {
    return length == ((ArrayType *)other)->getArrayLength();
  }
  return false;
}

} // namespace Types
} // namespace CCPP
