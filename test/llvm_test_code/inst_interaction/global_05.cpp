#include <iostream>

int g = 0;

void init() { g = 1; }

int foo() { return g; }

int main() {
  init();
  std::cout << foo();
}
