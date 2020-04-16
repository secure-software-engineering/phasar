#pragma once

namespace llvm {
class Value;
}

namespace psr::LCUtils {
bool isConstant(const llvm::Value *val);
}