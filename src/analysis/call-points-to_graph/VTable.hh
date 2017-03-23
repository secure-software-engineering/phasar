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
  int getEntryByFunction(string fname) const;
  void addEntry(string entry);
  vector<string> getVTable() const;
  friend ostream& operator<<(ostream& os, const VTable& t);
};

#endif /* ANALYSIS_VTABLE_HH_ */
