#include "Type.h"
namespace CCPP {
namespace Types {
class BaseType : public Type {

private:
  PrimitiveType prim;

public:
  BaseType(const std::string &name, bool isConst);
  BaseType(const std::string &name, PrimitiveType prim);
  virtual bool isPrimitiveType() const override;
  PrimitiveType getPrimitiveType() const;
  virtual bool canBeAssignedTo(Type *other) const override;
  virtual bool equivalent(Type *other) const override;
};
} // namespace Types
} // namespace CCPP