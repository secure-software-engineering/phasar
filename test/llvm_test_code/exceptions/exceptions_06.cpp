#include <exception>

extern void print(int) noexcept;

int foo(int p) {
  if (p > 100) {
    throw 3;
  }
  return p + 1;
}

int bar(int u) {
  try {
    return foo(u);
  } catch (...) {
    auto exc = std::current_exception();
    if (exc) {
      std::rethrow_exception(exc);
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int x;
  try {
    x = bar(argc);
    print(x); // x is BOTTOM here, as we don't know the value of argc
  } catch (int y) {
    x = y;
    print(x); // x is 3 here
  }

  print(x); // x is BOTTOM here = BOTTOM join 3
}