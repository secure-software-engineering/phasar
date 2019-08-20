#include "CrySLParser.h"
#include "Types/Type.h"
#include <memory>


namespace CCPP {
std::unique_ptr<Types::Type> &&
getOrCreateType(CrySLParser::TypeNameContext *ctx);

} // namespace CCPP
