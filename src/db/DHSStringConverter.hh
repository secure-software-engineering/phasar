/*
 * DHSStringConverter.hh
 *
 *  Created on: 24.05.2017
 *      Author: pdschbrt
 */

#ifndef DHSSTRINGCONVERTER_HH_
#define DHSSTRINGCONVERTER_HH_

#include <string>
#include "../analysis/ifds_ide/ZeroValue.hh"
#include "../utils/utils.hh"
using namespace std;


class DHSStringConverter {
public:
	~DHSStringConverter() = default;
	string DToHStoreStringRep(const llvm::Instruction* n, const llvm::Value* d);
	const llvm::Value* HStoreStringRepToD(const string& s);
};

#endif
