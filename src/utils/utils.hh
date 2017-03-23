#ifndef UTILS_HH
#define UTILS_HH

#include <algorithm>
#include <iostream>
using namespace std;

#define MYDEBUG

#define HEREANDNOW                                      \
  cerr << "file: " << __FILE__ << " line: " << __LINE__ \
       << " function: " << __func__ << endl;

#define KILL exit(-1);

#define CXXERROR(BOOL, STRING) \
  if (!BOOL) {                 \
    HEREANDNOW;                \
    cerr << STRING << endl;    \
    exit(-1);                  \
  }

ostream& operator<<(ostream& os, const vector<const char*> v);

#endif
