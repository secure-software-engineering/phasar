#ifndef UTILS_HH
#define UTILS_HH

#include <cxxabi.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
using namespace std;

#define MYDEBUG

#define HEREANDNOW                                      \
  cerr << "file: " << __FILE__ << " line: " << __LINE__ \
       << " function: " << __func__ << endl;

#define DIE_HARD exit(-1);

#define CXXERROR(BOOL, STRING) \
  if (!BOOL) {                 \
    HEREANDNOW;                \
    cerr << STRING << endl;    \
    exit(-1);                  \
  }

string cxx_demangle(string mangled_name);

extern const string MetaDataKind;

bool isFunctionPointer(const llvm::Value* V) noexcept;

bool matchesSignature(const llvm::Function* F, const llvm::FunctionType* FType);

#endif
