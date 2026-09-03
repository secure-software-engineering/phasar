#include "util.h"

#include <stdio.h>
int main(int argc, char **argv) {
  int x = compute(argc - 1);
  printf("%d\n", x);
  return 0;
}
