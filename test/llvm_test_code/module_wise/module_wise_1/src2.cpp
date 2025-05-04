#include "src2.h"

void leak_taint(int i) {
  // this should just provide a sink
}

int sanitize(int i) {
  if (i == 13) {
    return 0;
  } else {
    return i;
  }
}
