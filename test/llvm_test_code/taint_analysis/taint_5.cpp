#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  char *buffer;
  FILE *F;
  fread(buffer, 10, 10, F);
  return 0;
}
