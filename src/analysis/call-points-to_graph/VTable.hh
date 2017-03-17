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

class VTable {
 private:
  llvm::Type* type;
  vector<string> vtbl;

 public:
  VTable() = default;
  virtual ~VTable() = default;
  string getFunctionByEntry(unsigned i);
  void addEntry(string entry);
  friend ostream& operator<<(ostream& os, const VTable& t);
};

#endif /* ANALYSIS_VTABLE_HH_ */
