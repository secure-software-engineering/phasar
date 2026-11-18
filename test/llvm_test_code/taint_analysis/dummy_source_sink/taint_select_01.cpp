extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  int a = source();
  int b = argc > 1 ? a : 0;
  sink(b);
  return 0;
}
