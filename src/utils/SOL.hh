#ifndef SOL_HH_
#define SOL_HH_

#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace std;

class SOL {
 private:
  char *error;
  void *so_handle;

 public:
  SOL(const string &path);
  ~SOL();
  template <typename Signature>
  auto loadSymbol(const string &name) {
    auto sym = reinterpret_cast<Signature>(dlsym(so_handle, name.c_str()));
    if (!sym) throw invalid_argument("symbol not found");
    return sym;
  }
};

#endif