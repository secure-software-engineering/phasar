/* a | %1 (ID: 2) | mem2reg*/
#include <array>
int main() {
  std::array<int,3> a = {{1, 2, 3}};
  std::array<int,3> b = {{4, 5, 6}};
  a = b;
  return 0;
}