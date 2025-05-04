int g = 0;

__attribute__((constructor)) void global_ctor() { g = 42; }

int foo(int x) {
  x = x + 1;
  return x;
}

int main() {
  int a = g;
  a = foo(a);
  return a;
}
