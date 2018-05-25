int main(int argc, char **argv) {
  int a = 10;
  if (argc - 1) {
    a = 20;
  } else {
    a = 30;
  }
  int b = a + 42;
  return 0;
}