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
