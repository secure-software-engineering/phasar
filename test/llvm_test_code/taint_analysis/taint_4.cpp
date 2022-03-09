void someFunction(int i, int &j) { j = i; }

int main(int argc, char **argv) {
  int x;
  someFunction(argc, x);
  return 0;
}
