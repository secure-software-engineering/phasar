#include <cstdio>

void foo(int x, int &y) { y = x; }

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  PHASAR_DECLARE_COMPLETE_FUN_AS_SINK(printf);

  int x = 0;
  foo(argc, x);
  printf("%d\n", x);
}