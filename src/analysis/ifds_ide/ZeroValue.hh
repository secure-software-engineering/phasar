/*
 * ZeroValue.hh
 *
 *  Created on: 23.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_ZEROVALUE_HH_
#define SRC_ANALYSIS_IFDS_IDE_ZEROVALUE_HH_

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
using namespace std;

// do not touch, its only purpose is to make ZeroValue working
extern const string ZeroValueInternalName;
extern const string ZeroValueInternalModuleName;
extern const unique_ptr<llvm::LLVMContext> ZeroValueCTX;
extern const unique_ptr<llvm::Module> ZeroValueMod;

/**
 * This function can be used to determine if a Value is a ZeroVale.
 * Of course
 *
 * 	llvm::isa<ZeroValue>(V)
 *
 * may be used, but isZeroValue() is much cheaper since it
 * does not have to traverse the class hierarchy to check this.
 */
bool isLLVMZeroValue(const llvm::Value *V);

/**
 * This class may be used to represent the special zero value for IFDS
 * and IDE problems. Instances of this class must be allocated with new!
 *
 * 	ZeroValue *Z = new ZeroValue;
 *
 * The ZeroValue class does the clean-up itself, there are no memory leaks
 * even when a user allocates with new!!! The corresponding LLVMContext and
 * Module will do the clean-up for the user. A user is not allowed to call
 * delete on an allocated ZeroValue - it leads to misery and a double free
 * corruption! A ZeroValue may be dumped using 'dump()' which shall print
 * something similar to
 *
 * 	@zero_value = constant i2 0, align 4
 *
 * It makes much sense to use ZeroValue as a singleton, but one is not
 * restricted to that. Allocating more than one ZeroValue like
 *
 * 	ZeroValue *Z = new ZeroValue;
 *  ZeroValue *X = new ZeroValue;
 *	ZeroValue *Y = new ZeroValue;
 *
 * is allowed. In this case we can find such contents in memory
 *
 * 	@zero_value = constant i2 0, align 4
 *	@zero_value.1 = constant i2 0, align 4
 *	@zero_value.2 = constant i2 0, align 4
 */
class ZeroValue : public llvm::GlobalVariable {
public:
  ZeroValue();
  virtual ~ZeroValue() = default;
};

#endif /* SRC_ANALYSIS_IFDS_IDE_ZEROVALUE_HH_ */
