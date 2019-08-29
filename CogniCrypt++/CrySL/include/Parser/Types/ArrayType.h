#include "PointerType.h"

namespace CCPP {
namespace Types {
/// \brief A type representing C-style array types
class ArrayType : public PointerType {
  long long length;

public:
  ArrayType(std::shared_ptr<Type> &underlying, long long length);
  /// \brief The number of elements of arrays of this type
  long long getArrayLength() const;
  virtual bool equivalent(const Type *other) const override;
  virtual bool isArrayType() const override;
};
} // namespace Types
} // namespace CCPP