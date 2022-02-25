int main(int argc, char **argv) {
  int a = 10;
  int b = 100;
  if (argc - 1) {
    a = 20;
  } else {
    a = 30;
    b = 300;
  }
  return a + b;
}
