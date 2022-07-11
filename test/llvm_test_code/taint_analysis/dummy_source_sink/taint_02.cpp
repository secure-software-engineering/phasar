extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  sink(argc);
  return 0;
}
