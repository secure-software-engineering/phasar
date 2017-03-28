/*
 * VTable.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include "VTable.hh"

string VTable::getFunctionByEntry(unsigned i) {
  if (i < vtbl.size()) return vtbl[i];
  return "";
}

void VTable::addEntry(string entry) { vtbl.push_back(entry); }

ostream& operator<<(ostream& os, const VTable& t) {
  for_each(t.vtbl.begin(), t.vtbl.end(),
           [&](const string& entry) { os << entry << "\n"; });
  return os;
}

void VTable::addVTable(const VTable& vtbl_toadd) {
  vtbl.insert(vtbl.begin(), vtbl_toadd.vtbl.begin(), vtbl_toadd.vtbl.end());
}

int VTable::getEntryByFunction(string fname) const {
  auto iter = find(vtbl.begin(), vtbl.end(), fname);
  if (iter == vtbl.end()) {
    return -1;
  } else {
    return distance(vtbl.begin(), iter);
  }
}

vector<string> VTable::getVTable() const {
  return vtbl;
}

bool VTable::empty() {
  return vtbl.empty();
}