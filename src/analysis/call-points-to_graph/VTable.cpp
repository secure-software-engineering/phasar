/*
 * VTable.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include "VTable.hh"

string VTable::getFunctionByIdx(unsigned i) {
  if (i < vtbl.size()) return vtbl[i];
  return "";
}

void VTable::addEntry(string entry) { vtbl.push_back(entry); }

ostream& operator<<(ostream& os, const VTable& t) {
  for_each(t.vtbl.begin(), t.vtbl.end(),
           [&](const string& entry) { os << entry << string("\n"); });
  return os;
}

int VTable::getEntryByFunctionName(string fname) const {
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

json VTable::exportPATBCJSON() {
  json j = "{}"_json;
  for (unsigned idx = 0; idx < vtbl.size(); ++idx) {
    j.push_back({ to_string(idx), vtbl[idx] });
  }
  return j;
}

vector<string>::iterator VTable::begin() {
  return vtbl.begin();
}

vector<string>::const_iterator VTable::begin() const {
  return vtbl.begin();
}

vector<string>::iterator VTable::end() {
  return vtbl.end();
}

vector<string>::const_iterator VTable::end() const {
  return vtbl.end();
}
