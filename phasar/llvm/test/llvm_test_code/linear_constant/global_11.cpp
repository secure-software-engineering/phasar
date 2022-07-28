int g1 = 42;
int g2 = 9001;

int foo(int x) {
  x = x + 1;
  return x;
}

int main() {
  int a = 13;
  a = foo(a);
  return a;
}
