extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  int a = source();
  int b = 0;
  if (argc > 1) {
    b = a;
  }
  sink(b);
  return 0;
}
