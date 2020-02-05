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

#ifndef PHASAR_UTILS_LLVMSHORTHANDS_H_
#define PHASAR_UTILS_LLVMSHORTHANDS_H_

#include <string>
#include <vector>

#include <phasar/Utils/Utilities.h>

namespace llvm {
class Value;
class FunctionType;
class Function;
class Argument;
class Instruction;
class TerminatorInst;
class StoreInst;
class Module;
class StringRef;
} // namespace llvm

namespace psr {
/**
 * @brief Return type of SpecialMemberFunctionType.
 */
enum class SpecialMemberFunctionTy {
  NONE = 0,
  CTOR,
  DTOR,
  CPASSIGNOP,
  MVASSIGNOP
};

/**
 * @brief Hashes a string to an int so that strings can be used in switches.
 * @note In rare conditions the hash values of two strings can collide.
 */
constexpr unsigned int str2int(const char *str, int h);

/**
 * @brief checks if a function name is the name of a special member function.
 * @param s Mangled function name.
 * @return Returns an enum element of SpecialMemberFunctionTy.
 */
SpecialMemberFunctionTy specialMemberFunctionType(const std::string &s);

SpecialMemberFunctionTy specialMemberFunctionType(const llvm::StringRef &sr);

/**
 * @brief Checks if the given LLVM Value is a LLVM Function Pointer.
 * @param V LLVM Value.
 * @return True, if given LLVM Value is a LLVM Function Pointer. False,
 * otherwise.
 */
bool isFunctionPointer(const llvm::Value *V) noexcept;

/**
 * @brief Checks if the given LLVM Value is either a alloca instruction or a
 * heap allocation function, e.g. new, new[], malloc, realloc or calloc.
 */
bool isAllocaInstOrHeapAllocaFunction(const llvm::Value *V) noexcept;

// TODO add description
bool matchesSignature(const llvm::Function *F, const llvm::FunctionType *FType);

// TODO add description
bool matchesSignature(const llvm::FunctionType *FType1,
                      const llvm::FunctionType *FType2);

/**
 * @brief Returns a string representation of a LLVM Value.
 * @note Expensive function (between 20 to 550 ms per call)
 *       avoid to do it often, it can kill the performances (c.f. warning in the
 * implementation)
 */
std::string llvmIRToString(const llvm::Value *V);

/**
 * @brief Same as @link(llvmIRToString) but tries to shorten the
 *        resulting string
 */
std::string llvmIRToShortString(const llvm::Value *V);

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
struct llvmValueIDLess {
  stringIDLess sless;
  llvmValueIDLess();
  bool operator()(const llvm::Value *lhs, const llvm::Value *rhs) const;
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
                                             unsigned argNo);

/**
 * The Instruction count starts with one (not zero, as in Function arguments).
 *
 * @brief Returns the n-th LLVM Instruction of a given LLVM Function.
 * @param F Function to retrieve the Instruction from.
 * @param instNo Instruction number.
 * @return LLVM Instruction or nullptr, if instNo invalid.
 */
const llvm::Instruction *getNthInstruction(const llvm::Function *F,
                                           unsigned instNo);

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
                                               unsigned termInstNo);
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
                                              unsigned stoNo);

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
const std::string getModuleNameFromVal(const llvm::Value *V);

/**
 * @brief Computes a hash value for a given LLVM Module.
 * @param M LLVM Module.
 * @param considerIdentifier If true, module identifier will be considered for
 * hash computation.
 * @return Hash value.
 */
std::size_t computeModuleHash(llvm::Module *M, bool considerIdentifier);

/**
 * @brief Computes a hash value for a given LLVM Module.
 * @note Hash computation will consider the module identifier.
 * @param M
 * @return
 */
std::size_t computeModuleHash(const llvm::Module *M);

} // namespace psr

#endif
