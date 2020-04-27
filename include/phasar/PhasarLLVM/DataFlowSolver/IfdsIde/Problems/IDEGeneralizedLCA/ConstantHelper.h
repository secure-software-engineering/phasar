#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTHELPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_CONSTANTHELPER_H_

namespace llvm {
class Value;
}

namespace psr {
bool isConstant(const llvm::Value *val);
}

#endif
