#include "CrySLParser.h"
#include "Types/Type.h"
#include <memory>

namespace CCPP {
/// \brief Traverses the syntax structure of ctx for creating a Types::Type,
/// which represents it
std::shared_ptr<Types::Type> getOrCreateType(CrySLParser::TypeNameContext *ctx);

} // namespace CCPP
