int source() { return 0; } // dummy source
void sink(int p) {}        // dummy sink

int main(int argc, char **argv) {
  int a = source();
  sink(a);
  int b = a;
  sink(b);
  return 0;
}
