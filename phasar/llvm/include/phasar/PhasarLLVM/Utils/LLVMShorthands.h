/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMShorthands.h
 *
 *  Created on: 15.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_UTILS_LLVMSHORTHANDS_H
#define PHASAR_PHASARLLVM_UTILS_LLVMSHORTHANDS_H

#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseMap.h"

#include <string>
#include <vector>

namespace llvm {
class Value;
class Function;
class FunctionType;
class ModuleSlotTracker;
class Argument;
class Instruction;
class StoreInst;
class BranchInst;
class Module;
class CallInst;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

/**
 * @brief Checks if the given LLVM Value is a LLVM Function Pointer.
 * @param V LLVM Value.
 * @return True, if given LLVM Value is a LLVM Function Pointer. False,
 * otherwise.
 */
bool isFunctionPointer(const llvm::Value *V) noexcept;

/**
 * @brief Checks if the given LLVM Type is a integer like struct.
 * @param V LLVM Type.
 * @return True, if given LLVM Type is a struct like this %TSi = type <{ i64 }>.
 * False, otherwise.
 */
bool isIntegerLikeType(const llvm::Type *T) noexcept;

/**
 * @brief Checks if the given LLVM Value is either a alloca instruction or a
 * heap allocation function, e.g. new, new[], malloc, realloc or calloc.
 */
bool isAllocaInstOrHeapAllocaFunction(const llvm::Value *V) noexcept;

// TODO add description
bool matchesSignature(const llvm::Function *F, const llvm::FunctionType *FType,
                      bool ExactMatch = true);

// TODO add description
bool matchesSignature(const llvm::FunctionType *FType1,
                      const llvm::FunctionType *FType2);

llvm::ModuleSlotTracker &getModuleSlotTrackerFor(const llvm::Value *V);

/**
 * @brief Returns a string representation of a LLVM Value.
 */
[[nodiscard]] std::string llvmIRToString(const llvm::Value *V);

/**
 * @brief Similar to llvmIRToString, but removes the metadata from the output as
 * they are not always stable. Prefer this function over llvmIRToString, if you
 * are comparing the string representations of LLVM iR instructions.
 */
[[nodiscard]] std::string llvmIRToStableString(const llvm::Value *V);

/**
 * @brief Same as @link(llvmIRToString) but tries to shorten the
 *        resulting string
 */
std::string llvmIRToShortString(const llvm::Value *V);

/**
 * @brief Returns a string-representation of a LLVM type.
 *
 * @param Shorten Tries to shorten the output
 */
[[nodiscard]] std::string llvmTypeToString(const llvm::Type *Ty,
                                           bool Shorten = false);

LLVM_DUMP_METHOD void dumpIRValue(const llvm::Value *V);
LLVM_DUMP_METHOD void dumpIRValue(const llvm::Instruction *V);

/**
 * @brief Returns all LLVM Global Values that are used in the given LLVM
 * Function.
 */
std::vector<const llvm::Value *>
globalValuesUsedinFunction(const llvm::Function *F);

/**
 * Only Instructions and GlobalVariables have 'real' ID's, i.e. annotated meta
 * data. Formal arguments cannot be annotated with metadata in LLVM. Therefore,
 * a formal arguments ID will look like this:
 *    <function_name>.<#argument>
 *
 * ZeroValue will have -1 as ID by default.
 *
 * @brief Returns the ID of a given LLVM Value.
 * @return Meta data ID as a string or -1, if it's not
 * an Instruction, GlobalVariable or Argument.
 */
std::string getMetaDataID(const llvm::Value *V);

/**
 * @brief Does less-than comparison based on the annotated ID.
 *
 * This is useful, since Instructions/Globals and Arguments have different
 * underlying types for their ID's, size_t and string respectively.
 */
struct LLVMValueIDLess {
  bool operator()(const llvm::Value *Lhs, const llvm::Value *Rhs) const;
};

/**
 * @brief Returns position of a formal function argument.
 * @param Arg LLVM Argument.
 * @return Position or -1 if argument does not belong to any function.
 */
int getFunctionArgumentNr(const llvm::Argument *Arg);

/**
 * The Argument count starts with 0.
 *
 * @brief Returns the n-th LLVM Argument of a given LLVM Function.
 * @param F Function to retrieve the Arguments from.
 * @param argNo Argument number.
 * @return LLVM Argument or nullptr, if argNo invalid.
 */
const llvm::Argument *getNthFunctionArgument(const llvm::Function *F,
                                             unsigned ArgNo);

/**
 * The Instruction count starts with one (not zero, as in Function arguments).
 *
 * @brief Returns the n-th LLVM Instruction of a given LLVM Function.
 * @param F Function to retrieve the Instruction from.
 * @param instNo Instruction number.
 * @return LLVM Instruction or nullptr, if instNo invalid.
 */
const llvm::Instruction *getNthInstruction(const llvm::Function *F,
                                           unsigned Idx);

const llvm::Instruction *getLastInstructionOf(const llvm::Function *F);

/**
 * The Termination Instruction count starts with one (not zero, as in Function
 * arguments).
 *
 * @brief Returns the n-th LLVM Termination Instruction of a given LLVM
 * Function.
 * @param F Function to retrieve the Termination Instruction from.
 * @param termInstNo Termination Instruction number.
 * @return LLVM Instruction or nullptr, if termInstNo invalid.
 */
const llvm::Instruction *getNthTermInstruction(const llvm::Function *F,
                                               unsigned TermInstNo);
/**
 * The Store Instruction count starts with one (not zero, as in Function
 * arguments).
 *
 * @brief Returns the n-th LLVM Store Instruction of a given LLVM
 * Function.
 * @param F Function to retrieve the Store Instruction from.
 * @param termInstNo Store Instruction number.
 * @return LLVM Store Instruction or nullptr, if stoNo invalid.
 */
const llvm::StoreInst *getNthStoreInstruction(const llvm::Function *F,
                                              unsigned StoNo);

llvm::SmallVector<const llvm::Instruction *, 2>
getAllExitPoints(const llvm::Function *F);
void appendAllExitPoints(
    const llvm::Function *F,
    llvm::SmallVectorImpl<const llvm::Instruction *> &ExitPoints);

/**
 * @brief Returns the LLVM Module to which the given LLVM Value belongs to.
 * @param V LLVM Value.
 * @return LLVM Module or nullptr.
 */
const llvm::Module *getModuleFromVal(const llvm::Value *V);

/**
 * @brief Returns the name of the LLVM Module to which the given LLVM Value
 * belongs to.
 * @param V LLVM Value.
 * @return Module name or empty string.
 */
std::string getModuleNameFromVal(const llvm::Value *V);

/**
 * @brief Computes a hash value for a given LLVM Module.
 * @param M LLVM Module.
 * @param considerIdentifier If true, module identifier will be considered for
 * hash computation.
 * @return Hash value.
 */
std::size_t computeModuleHash(llvm::Module *M, bool ConsiderIdentifier);

/**
 * @brief Computes a hash value for a given LLVM Module.
 * @note Hash computation will consider the module identifier.
 * @param M
 * @return
 */
std::size_t computeModuleHash(const llvm::Module *M);

/**
 * @brief True, iff V is the compiler-generated guard variable for the
 * thread-safe initialization of function-local static variables.
 */
bool isGuardVariable(const llvm::Value *V);

/**
 * @brief True, iff V is the compiler-generated branch that leads to the lazy
 * initialization of a function-local static variable.
 */
bool isStaticVariableLazyInitializationBranch(const llvm::BranchInst *Inst);

/**
 * Tests for https://llvm.org/docs/LangRef.html#llvm-var-annotation-intrinsic
 * e.g.
 * int boo __attribute__((annotate("bar"));
 * @param F The function to test - Target of the call instruction
 */
bool isVarAnnotationIntrinsic(const llvm::Function *F);

/**
 * Retrieves String annotation value as per
 * https://llvm.org/docs/LangRef.html#llvm-var-annotation-intrinsic
 * Test the call function be tested by isVarAnnotationIntrinsic
 *
 */
llvm::StringRef getVarAnnotationIntrinsicName(const llvm::CallInst *CallInst);

class ModulesToSlotTracker {
  friend class LLVMProjectIRDB;
  friend class LLVMBasedICFG;
  friend class LLVMZeroValue;

private:
  static void setMSTForModule(const llvm::Module *Module);
  static void updateMSTForModule(const llvm::Module *Module);
  static void deleteMSTForModule(const llvm::Module *Module);

public:
  static llvm::ModuleSlotTracker &
  getSlotTrackerForModule(const llvm::Module *Module);
};
} // namespace psr

#endif
