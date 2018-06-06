/* - | - | mem2reg*/
#include <array>
int main() {
  std::array<int,3> a;
  std::array<int,3> b = {{4, 5, 6}};
  a = b;
  return 0;
}