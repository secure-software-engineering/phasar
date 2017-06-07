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

template <typename D>
class DHSStringConverter {
	virtual ~DHSStringConverter() = default;
	virtual string DToHStoreStringRep(const llvm::Instruction* n, D d) = 0;
	virtual D HStoreStringRepToD(const string& s) = 0;
};

template <>
class DHSStringConverter<const llvm::Value*> {
public:
	~DHSStringConverter() = default;
	string DToHStoreStringRep(const llvm::Instruction* n, const llvm::Value* d);
	const llvm::Value* HStoreStringRepToD(const string& s);
};

#endif
