#include <stdio.h>
#include <stdlib.h>

int *bar(int *Foo, int I) {
  if (I > 10) {
    return Foo;
  }
  return 0;
}

void lorem(int I) {
  if (I > 10) {
  }
}

int main(int Argc, char **Argv) {
  int K = 0, J = 10;
  int Z = Argc;
  lorem(Z);
  if (K == 0) {
    J = 20;
  }
  int *Foo = &Z;
  int M = *bar(Foo, Z);
  if (M < 0) {
    J = 42;
  }
  return J;
}
