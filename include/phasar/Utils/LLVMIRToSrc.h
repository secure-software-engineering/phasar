/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIRToSrc.h
 *
 *  Created on: 11.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_UTILS_LLVMIRTOSRC_H_
#define PHASAR_UTILS_LLVMIRTOSRC_H_

#include <string>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Argument;
class Instruction;
class Function;
class Value;
class GlobalVariable;
class Module;
} // namespace llvm

namespace psr {

/**
 * @brief Maps the given @see llvm::Argument to corresponding formal parameter
 * of a function in source code.
 */
std::string llvmArgumentToSrc(const llvm::Argument *A, bool ScopeInfo = true);

/**
 * @brief Maps the given @see llvm::Instruction to corresponding instruction in
 * source code.
 */
std::string llvmInstructionToSrc(const llvm::Instruction *I,
                                 bool ScopeInfo = true);

/**
 * @brief Maps the given @see llvm::Function to corresponding function in source
 * code.
 */
std::string llvmFunctionToSrc(const llvm::Function *F);

/**
 * @brief Maps the given @see llvm::GlobalVariable to corresponding global
 * variable in source code.
 */
std::string llvmGlobalValueToSrc(const llvm::GlobalVariable *GV);

/**
 * @brief Maps the given llvm::Module to corresponding compile unit.
 */
std::string llvmModuleToSrc(const llvm::Module *M);

/**
 * @brief Maps the given llvm::Value to corresponding source information.
 */
std::string llvmValueToSrc(const llvm::Value *V, bool ScopeInfo = true);

} // namespace psr

#endif
