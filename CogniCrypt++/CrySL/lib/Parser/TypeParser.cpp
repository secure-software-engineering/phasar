#include <Parser/TypeParser.h>
#include <Parser/Types/BaseType.h>
#include <Parser/Types/PointerType.h>
#include <vector>

namespace CCPP {
using namespace Types;

Type::PrimitiveType parsePrimitive(CrySLParser::PrimitiveTypeNameContext *ctx) {
  if (ctx->booleanType)
    return Type::PrimitiveType::BOOL;

  if (ctx->floatingPoint)
    return Type::PrimitiveType::FLOAT;
  if (ctx->doubleFloat) {
    if (ctx->longDouble)
      return Type::PrimitiveType::LONGDOUBLE;
    else
      return Type::PrimitiveType::DOUBLE;
  }
  if (ctx->sizeType)
    return Type::PrimitiveType::SIZE_T;

  // else: integer

  Type::PrimitiveType ret;
  if (ctx->charTy)
    ret = Type::PrimitiveType::UCHAR;
  else if (ctx->shortTy)
    ret = Type::PrimitiveType::USHORT;
  else if (ctx->intTy)
    ret = Type::PrimitiveType::UINT;
  else if (ctx->longTy)
    ret = Type::PrimitiveType::ULONG;
  else if (ctx->longlongTy)
    ret = Type::PrimitiveType::ULONGLONG;
  if (!ctx->unsignedInt)
    ret = (Type::PrimitiveType)(ret - 1);
  return ret;
}

std::shared_ptr<Type> getOrCreateType(CrySLParser::TypeNameContext *ctx,
                                      bool isConst) {
  std::shared_ptr<Type> ty;
  if (ctx->qualifiedName()) {
    auto name = ctx->qualifiedName()->getText();
    ty = std::shared_ptr<Type>((Type *)new BaseType(name, isConst));
  } else {
    auto prim = parsePrimitive(ctx->primitiveTypeName());
    auto name = ctx->primitiveTypeName()->getText();
    ty = getOrCreatePrimitive(name, prim);
  }
  auto ptrCount = ctx->ptr().size();
  for (size_t i = 0; i < ptrCount; ++i) {
    ty = std::shared_ptr<Type>((Type *)new PointerType(ty));
  }
  return ty;
}

std::shared_ptr<Types::Type>
getOrCreatePrimitive(const std::string &name, Types::Type::PrimitiveType prim) {
  // We have 18 different primitive types (including NONE)
  static std::vector<std::shared_ptr<Types::Type>> primCache(18, nullptr);
  auto &ret = primCache[prim];
  if (!ret) {
    ret = std::shared_ptr<Type>((Type *)new BaseType(name, prim));
  }
  return ret;
}
std::shared_ptr<Types::Type>
createPrimitivePointerType(const std::string &name,
                           Types::Type::PrimitiveType prim) {
  auto ret = getOrCreatePrimitive(name, prim);
  return std::shared_ptr<Type>((Type *)new PointerType(ret));
}
} // namespace CCPP