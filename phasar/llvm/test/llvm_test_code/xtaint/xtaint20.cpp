extern void srcsink(
    [[clang::annotate("psr.sink")]] [[clang::annotate("psr.source")]] int &) {}
void sink([[clang::annotate("psr.sink")]] int) {}

int main(int argc, char *argv[]) {
  int x = 42;
  int y = 24;

  srcsink(x);
  srcsink(y);

  srcsink(x); // leak
  sink(y);    // leak
}
