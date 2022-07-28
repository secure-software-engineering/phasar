#include <iostream>

int main(int argc, char **argv) {
  int i = 13;
  int j = i + 42;
  int k = 0;
  if (argc > 1) {
    k = j;
  }
  std::cout << k;
  return 0;
}
