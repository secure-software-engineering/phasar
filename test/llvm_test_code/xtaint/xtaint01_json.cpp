
extern void print(int);

int main(int argc, char *argv[]) {
  // PHASAR_DECLARE_FUN_AS_SINK(print, 0);
  // PHASAR_DECLARE_VAR_AS_SOURCE(argc);

  print(argc);
}
