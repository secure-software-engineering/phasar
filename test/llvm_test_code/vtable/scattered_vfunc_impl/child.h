#ifndef CHILD_H_
#define CHILD_H_

#include "base.h"

struct child : base {
  virtual int foo() override;
  virtual int bar() override;
  int other();
};

#endif
