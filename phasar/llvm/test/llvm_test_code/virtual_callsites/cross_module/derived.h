#ifndef DERIVED_H_
#define DERIVED_H_

#include "base.h"

struct derived : base {
  void foo();
  virtual int bar();
};

#endif
