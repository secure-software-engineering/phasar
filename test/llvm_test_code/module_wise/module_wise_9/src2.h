#ifndef src2_H_
#define src2_H_

#include "abst.h"

struct OtherConcrete : Abstract {
  void foo(int &i) override;
};

#endif
