int foo(int a) {
  a = 10;
  return 44;
}

int main() {
  int i = 10;
  int k = foo(i);
  return 0;
}