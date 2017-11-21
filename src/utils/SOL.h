#ifndef SOL_H_
#define SOL_H_

#include "Logger.h"
#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
using namespace std;

class SOL {
private:
  char *error;
  void *so_handle;
  const string path;

public:
  SOL(const string &path);
  ~SOL();
  template <typename Signature> auto loadSymbol(const string &name) {
    auto sym = reinterpret_cast<Signature>(dlsym(so_handle, name.c_str()));
    if (!sym)
      throw invalid_argument("symbol not found");
    return sym;
  }
};

#endif