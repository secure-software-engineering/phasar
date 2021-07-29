int g = 15;

int foo(int x) {
  x = x + 1;
  return x;
}

int main() {
  int a = g;
  a = foo(a);
  return a;
}
