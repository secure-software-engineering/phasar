#include "src1.h"

int generate_taint() {
  // 13 is an evil tainted value
  return 13;
}

int sanitize(int i) {
  if (i == 13) {
    return 0;
  } else {
    return i;
  }
}

void leak_taint(int i) {
  // just provide a name for a sink
}
