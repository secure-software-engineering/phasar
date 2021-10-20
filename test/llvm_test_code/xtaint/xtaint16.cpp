void sink([[clang::annotate("psr.sink")]] int) {}
extern int rand();

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {

  int x = argc;
  while (rand()) {
    x = 0;
    break;
  }

  // we can skip the sanitizer => leak here
  sink(x);
}