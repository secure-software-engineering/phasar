#include <cstdio>
#include <cstdlib>

extern void foo() noexcept;

void usage(int p) {
  int q = p + 1;
  foo();
  puts("Usage: external_fun <arg>");
  abort();
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    usage(0);
  }
  int x = 42;
  int y = x + 1;
  int z = y * 2;
  return y;
}
