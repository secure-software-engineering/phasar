#include <stdio.h>

int main(int argc, char **argv) {
  FILE *f;
  f = fopen("file.txt", "r");
  if (argv - 1) {
    fclose(f);
  }
  return 0;
}