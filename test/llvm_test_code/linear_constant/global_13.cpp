int g = 0;

__attribute__((constructor)) void global_ctor() { g = 42; }

__attribute__((destructor)) void global_dtor() { g = 666; }

int foo(int x) {
  x = x + 1;
  return x;
}

int main() {
  int a = g;
  int b = foo(a);
  return g;
}
