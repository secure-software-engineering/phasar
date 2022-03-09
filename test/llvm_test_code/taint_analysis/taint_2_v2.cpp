#include <iostream>
void someFunction(int i) { std::cout << i << std::endl; }

int main(int argc, char **argv) {
  someFunction(argc);
  return 0;
};
