#pragma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <memory>
#include<phasar/PhasarLLVM>

#include <string>

namespace CCPP {

class ObjectConverter {
private:
  llvm::Value *val;

public:
  CCPP::Types::Type getType();
  bool operator==(const ObjectConverter &oc);
};
} // namespace CCPP