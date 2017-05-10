/*
 * VTable.hh
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_VTABLE_HH_
#define ANALYSIS_VTABLE_HH_

#include <llvm/IR/Type.h>
#include <algorithm>
#include <string>
#include <vector>
using namespace std;

/**
 * 	@brief Represents a virtual method table.
 */
class VTable {
 private:
  llvm::Type* type;
  vector<string> vtbl;

 public:
  VTable() = default;
  virtual ~VTable() = default;
  string getFunctionByIdx(unsigned i);
  int getEntryByFunctionName(string fname) const;
  void addEntry(string entry);
  bool empty();
  vector<string> getVTable() const;
  friend ostream& operator<<(ostream& os, const VTable& t);
};

#endif /* ANALYSIS_VTABLE_HH_ */
