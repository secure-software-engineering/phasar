#include "utils.hh"

ostream& operator<<(ostream& os, const vector<const char*> v) {
  for_each(v.begin(), v.end(), [&os](const char* str) { os << str << " "; });
  return os;
}