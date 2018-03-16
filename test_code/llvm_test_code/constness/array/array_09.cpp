/* immutable: j */
// #include <iostream>
int main() {
  int i = 99, j = 77; 
  int *parr[2] = {&i, &j};
  *parr[0] = 42;
  // std::cout << i << " " << j << std::endl; // 42 77
  return 0;
}