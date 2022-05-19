#include "nlohmann/json.hpp"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const nlohmann::json &J) {
  // do the actual serialization
  nlohmann::detail::serializer<nlohmann::json> S(
      nlohmann::detail::llvm_output_adapter(O), ' ');
  S.dump(J, false, false, 0);
  return O;
}

} // namespace psr
