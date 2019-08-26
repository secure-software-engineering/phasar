#include <cstdio>
void someFunction(int i) { printf("%d\n", i); }

int main(int argc, char **argv) {
  someFunction(argc);
  return 0;
};