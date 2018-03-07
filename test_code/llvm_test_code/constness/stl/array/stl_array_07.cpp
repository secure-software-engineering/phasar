/* immutable: A, B */
#include <array>
int main() {
  std::array<int,3> A;
  std::array<int,3> B = {{4, 5, 6}};
  A = B;
  return 0;
}