#include <cstdio>
#include <cstdlib>

[[clang::annotate("psr.source")]] extern int source() { return 0; }
void sink([[clang::annotate("psr.sink")]] int) {}

int main() {
  auto mem = (int *)malloc(sizeof(int));
  *mem = source();

  if (rand())
    *mem = 42;

  sink(*mem);

  free(mem);
}
