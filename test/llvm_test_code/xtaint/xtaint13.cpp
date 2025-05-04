[[clang::annotate("psr.source")]] extern int source() { return 0; }
void sink([[clang::annotate("psr.sink")]] int) {}
void sanitize([[clang::annotate("psr.sanitizer")]] int *) noexcept {}

struct DoubleIntPair {
  double d;
  int i;
};

int main() {
  DoubleIntPair dip = {3.1415926, source()};

  auto x = dip.i;
  sanitize(&dip.i);

  sink(dip.i);
  sink(x);
}
