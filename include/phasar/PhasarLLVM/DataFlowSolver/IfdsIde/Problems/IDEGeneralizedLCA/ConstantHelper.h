#pragma once

namespace llvm {
class Value;
}

namespace psr {
bool isConstant(const llvm::Value *val);
}
