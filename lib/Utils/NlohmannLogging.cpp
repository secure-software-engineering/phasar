
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

namespace psr {

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const nlohmann::json &J) {
  // do the actual serialization
  nlohmann::detail::serializer<nlohmann::json> S(
      nlohmann::detail::llvm_output_adapter(OS), ' ');
  S.dump(J, false, false, 0);
  return OS;
}

} // namespace psr
