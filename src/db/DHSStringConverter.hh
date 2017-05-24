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
using namespace std;

/**
 * This interface allows the conversion of data-flow facts into
 * a adequate string representation that allow the storage of the
 * facts in Hexastores.
 */
template <typename N, typename D>
class DHSStringConverter {
public:
	virtual ~DHSStringConverter() = default;
	virtual string DToHStoreStringRep(N n, D d) = 0;
	virtual D HStoreStringRepToD(const string& s) = 0;
};

/**
 * This is a template specialization of DHSStringConverter that pre-defines
 * the interface for type D = const llvm::Value* which might be the
 * easiest and most common application of the persisted storage of
 * data-flow facts in a Hexastore.
 */
template <>
class DHSStringConverter<const llvm::Instruction*, const llvm::Value*> {
public:
	virtual ~DHSStringConverter() = default;
	virtual string DToHStoreStringRep(const llvm::Instruction* n, const llvm::Value* d) {
		return "";
	}
	virtual const llvm::Value* HStoreStringRepToD(const string& s) {
		return nullptr;
	}
};

#endif
