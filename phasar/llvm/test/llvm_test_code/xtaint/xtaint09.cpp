#include <cstdio>
#include <cstdlib>
#include <memory>

[[clang::annotate("psr.source")]] extern int source() { return 0; }
void sink([[clang::annotate("psr.sink")]] int) {}

int main() {

  auto mem = std::make_unique<int>();
  *mem = source();

  if (rand())
    *mem = 42;

  sink(*mem);
}
