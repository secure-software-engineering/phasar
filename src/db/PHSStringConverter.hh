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
 * Allows the (de-)serialization of Instructions, Arguments and GlobalValue
 * into HexaStore string representation.
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
 */
class PHSStringConverter {
private:
	ProjectIRCompiledDB& IRDB;

public:
	PHSStringConverter(ProjectIRCompiledDB& IRDB);
	~PHSStringConverter() = default;
  string PToHStoreStringRep(const llvm::Value *V);
  const llvm::Value *HStoreStringRepToP(const string &S);
};

#endif
