#include <iostream>

int main() {
  char data[64] = "Hello, World!";
  char first = data[0];
  data[0] = 'h';
  std::cout << data << '\n';
}
