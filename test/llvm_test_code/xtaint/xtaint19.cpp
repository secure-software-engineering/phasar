void sink([[clang::annotate("psr.sink")]] int) {}
extern int rand();

int foo(int x) {
  while (rand()) {
    x = 0;
    break;
  }
  return x;
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {

  int x = foo(argc);

  // we can skip the sanitizer => leak here
  sink(x);
}
