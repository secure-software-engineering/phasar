#include "src2.h"

void bar(int &i) {
  i = 12;
  tar(i);
}

void tar(int &i) { i = 3; }
