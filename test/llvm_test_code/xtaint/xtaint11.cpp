#include <cstdio>
#include <cstdlib>
#include <memory>

[[clang::annotate("psr.source")]] extern int source() { return 0; }
void sink([[clang::annotate("psr.sink")]] int) {}
void sanitize([[clang::annotate("psr.sanitizer")]] int &) noexcept {}

struct IntPair {
  int x;
  int y;
};

int main() {
  auto mem = std::make_unique<IntPair>();
  mem->x = source();

  // if (rand())
  //   mem->y = 42;

  sanitize(mem->x);

  sink(mem->x);
  sink(mem->y);
}