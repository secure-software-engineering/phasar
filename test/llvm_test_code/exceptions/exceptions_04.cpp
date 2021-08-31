/// LCA example

extern void print(int);
extern "C" int rand(void);

int g = 3;

__attribute__((optnone)) int baz(int p) {
  if (rand()) {
    return p + 1;
  }

  g++;
  throw p - 1;
}

__attribute__((optnone)) int bar(int u) { return baz(u); }

__attribute__((optnone)) int foo(int v) { return bar(v); }

int main() {
  int x = 42;

  try {
    x = foo(x);
    print(x); // should print 43
    print(g); // should print 3
  } catch (int y) {
    print(y); // should print 41
    print(g); // should print 4
  }
}