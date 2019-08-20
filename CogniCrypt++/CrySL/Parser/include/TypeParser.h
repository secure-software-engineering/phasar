#include "CrySLParser.h"
#include "Types/Type.h"
#include <memory>

namespace CCPP {
std::shared_ptr<Types::Type> getOrCreateType(CrySLParser::TypeNameContext *ctx);

} // namespace CCPP
