int foo() {
  static int x = 3;
  return ++x;
}

int main() {
  int i = foo();
  int j = foo();
  int k = foo();
  return k;
}