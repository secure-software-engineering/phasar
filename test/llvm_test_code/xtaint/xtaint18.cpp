void sink([[clang::annotate("psr.sink")]] int) {}
extern int rand();

int foo(int x) {
  if (rand())
    return 0;

  return foo(x);
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {

  int x = foo(argc);

  // here, the sanitizer cannot be skipped...
  sink(x);
}
