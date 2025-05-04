int id(int a) { return a; }

int main() {
  int a = -1;
  a = id(a);
  int b = 42;
  b = id(42);
  return 0;
}
