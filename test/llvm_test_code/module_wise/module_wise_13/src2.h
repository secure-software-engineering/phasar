#ifndef SRC2_H_
#define SRC2_H_

#include "src1.h"

struct B : A {
  virtual int foo(int &i) override;
};

#endif
