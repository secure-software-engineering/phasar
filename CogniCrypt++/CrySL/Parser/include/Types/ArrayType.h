#include "PointerType.h"

namespace CCPP {
namespace Types {
class ArrayType : public PointerType {
  long long length;

public:
  ArrayType(std::shared_ptr<Type> &underlying, long long length);
  long long getArrayLength() const;
  virtual bool equivalent(Type *other) const override;
  virtual bool isArrayType() const override;
};
} // namespace Types
} // namespace CCPP