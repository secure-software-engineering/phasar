/*
 * PHSStringConverter.hh
 *
 *  Created on: 01.06.2017
 *      Author: pdschbrt
 */

#ifndef PHSSTRINGCONVERTER_HH_
#define PHSSTRINGCONVERTER_HH_

#include "../utils/Configuration.hh"
#include "../utils/utils.hh"
#include "../db/ProjectIRCompiledDB.hh"
#include "../lib/LLVMShorthands.hh"
#include "../analysis/ifds_ide/ZeroValue.hh"
#include <string>
using namespace std;

/**
 * Allows the (de-)serialization of Instructions, Arguments, GlobalValues and
 * Operands into unique Hexastore string representation.
 *
 * What values can be serialized and what scheme is used?
 *
 * 	1. Instructions
 *
 * 		<function name>.<id>
 *
 * 	2. Formal parameters
 *
 *		<function name>.f<arg-no>
 *
 *	3. Global variables
 *
 *		<global variable name>
 *
 *	4. ZeroValue
 *
 *		<ZeroValueInternalName>
 *
 *	5. Operand of an instruction
 *
 *		<function name>.<id>.o.<operand no>
 *
 * @brief Provides operations to create unique string representations of
 *        llvm::Values (and vice versa) which are used in a Hexastore.
 */
class PHSStringConverter {
private:
	ProjectIRCompiledDB& IRDB;

public:
	/**
	 * @brief Creates an object of the converter.
	 * @param IRDB Holds the neccessary information for (de-)serialization.
	 */
	PHSStringConverter(ProjectIRCompiledDB& IRDB);

	~PHSStringConverter() = default;

	/**
	 * @brief Creates a unique string representation of a given llvm::Value.
	 */
  string PToHStoreStringRep(const llvm::Value *V);

	/**
	 * @brief Convertes the given string back into the llvm::Value it represents.
	 * @return Pointer to the converted llvm::Value.
	 */
	const llvm::Value *HStoreStringRepToP(const string &S);
};

#endif
