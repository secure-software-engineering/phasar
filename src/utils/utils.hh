#ifndef UTILS_HH
#define UTILS_HH

#include <cxxabi.h>
#include <algorithm>
#include <iostream>
#include <string>
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

string cxx_demangle(string mangled_name);

#endif
