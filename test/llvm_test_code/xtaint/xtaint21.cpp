extern void srcsink(int &);
extern void sink(int);

int main(int argc, char *argv[]) {
  // The configuration is provided as callback
  //
  // PHASAR_DECLARE_FUN_AS_SINK(srcsink, 0);
  // PHASAR_DECLARE_FUN_AS_SINK(sink, 0);
  // PHASAR_DECLARE_FUN_AS_SOURCE(srcsink, false, 0);

  int x = 42;
  int y = 24;

  srcsink(x);
  srcsink(y);

  srcsink(x); // leak
  sink(y);    // leak
}
