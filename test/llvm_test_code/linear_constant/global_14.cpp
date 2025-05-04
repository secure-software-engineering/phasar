int g = 0;

struct X {
  X() { g = 1024; }
};

int foo(int x) {
  x = x + 1;
  return x;
}

X var;

int main() {
  int a = g;
  a = foo(a);
  return a;
}
