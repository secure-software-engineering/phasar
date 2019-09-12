#include <Parser/CrySLParser.h>
#include <Parser/Types/Type.h>
#include <memory>

namespace CCPP {
/// \brief Traverses the syntax structure of ctx for creating a Types::Type,
/// which represents it
std::shared_ptr<Types::Type> getOrCreateType(CrySLParser::TypeNameContext *ctx,
                                             bool isConst);
std::shared_ptr<Types::Type>
getOrCreatePrimitive(const std::string &name, Types::Type::PrimitiveType prim);
std::shared_ptr<Types::Type>
createPrimitivePointerType(const std::string &name,
                           Types::Type::PrimitiveType prim);
} // namespace CCPP
