#ifndef UTILS_HH
#define UTILS_HH

#include <iostream>
#include <algorithm>
using namespace std;

#define MYDEBUG

#define HEREANDNOW                                               \
  cerr << "error in file: " << __FILE__ << " line: " << __LINE__ \
       << " function: " << __func__ << endl;

#define CXXERROR(BOOL, STRING) \
  if (!BOOL) {                 \
    HEREANDNOW;                \
    cerr << STRING << endl;    \
    exit(-1);                  \
  }

ostream& operator<<(ostream& os, const vector<const char*> v);

#endif
