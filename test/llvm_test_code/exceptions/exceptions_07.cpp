// TaintAnalysis example

extern "C" int rand(void);

[[clang::annotate("psr.source")]] int source() noexcept {}
void sink([[clang::annotate("psr.sink")]] int) noexcept {}

int foo(int p) {
  if (rand()) {
    return p;
  }

  throw source();
}

int main() {
  int x = 3;
  try {
    x = foo(x);
    sink(x); // no leak
  } catch (int y) {
    x = y;
  }

  sink(x); // leak through y
}