#pragma once
namespace llvm {
class Value;
}
namespace CCPP::LCUtils {
bool isConstant(const llvm::Value *val);
}