extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  int a = source();
  int b = argc > 1 ? 1 : 2;
  sink(b);
  sink(a);
  return 0;
}
