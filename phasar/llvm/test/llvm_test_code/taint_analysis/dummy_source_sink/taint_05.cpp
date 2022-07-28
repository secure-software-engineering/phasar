int source() { return 0; }      // dummy source
void sink(int p) { int b = p; } // dummy sink

int main(int argc, char **argv) {
  int a = source();
  sink(a);
  return 0;
}
