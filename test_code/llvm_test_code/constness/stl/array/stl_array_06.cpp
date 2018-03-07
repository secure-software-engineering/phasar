/* immutable: - */
#include <array>
int main() {
  std::array<int,3> A = {{1, 2, 3}};
  std::array<int,3> B = {{4, 5, 6}};
  A.swap(B);  // A and B mutable
  return 0;
}