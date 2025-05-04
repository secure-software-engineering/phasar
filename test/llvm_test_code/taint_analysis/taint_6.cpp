#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  FILE *F;
  fwrite(argv[0], 10, 10, F);
  return 0;
}
