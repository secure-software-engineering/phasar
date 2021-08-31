/// LCA example

extern void print(int);
extern "C" int rand(void);

int foo() {
  if (rand()) {
    return 42;
  }

  throw 120;
}

int main() {
  int x;
  try {
    x = foo();
    print(x); // Here, x is 42
  } catch (int y) {
    x = y;
    print(x); // Here, x is 120
  }
}