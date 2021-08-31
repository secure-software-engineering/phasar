// TaintAnalysis example

extern "C" int rand(void);

[[clang::annotate("psr.source")]] int source() noexcept {}
void sink([[clang::annotate("psr.sink")]] int) noexcept {}
void sanitize([[clang::annotate("psr.sanitizer")]] int &) noexcept {}

int foo(int p) {
  if (rand()) {
    sanitize(p);
  } else {
    throw 42;
  }

  return p;
}

int main() {
  int x = source();
  try {
    x = foo(x);
    sink(x); // should not leak
  } catch (...) {
  }
  sink(x); // Should leak, when coming from the catch-block
}