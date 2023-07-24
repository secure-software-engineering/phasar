int g1 = 0;
int g2 = 99;

struct X {
  X() { g1 = 1024; }
};

struct Y {
  Y() { g2 = g2 + 1; }
  ~Y() { g1 = g2 + 13; }
};

int foo(int x) {
  x = x + 1;
  return x;
}

X var;
Y war;

int main() {
  int a = g1;
  int b = g2;
  a = foo(a);
  return a + 30;
}
