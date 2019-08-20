#include "TypeParser.h"
#include <Types/BaseType.h>
#include <Types/PointerType.h>

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
    ty = std::shared_ptr<Type>((Type *)new BaseType(name, prim));
  }
  auto ptrCount = ctx->ptr().size();
  for (size_t i = 0; i < ptrCount; ++i) {
    ty = std::shared_ptr<Type>((Type *)new PointerType(ty));
  }
  return ty;
}
} // namespace CCPP