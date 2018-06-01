/* - | - | mem2reg*/
#include <array>
int main() {
  std::array<int,3> a;
  a[0] = 20;
  a.at(1) = 13;
  return 0;
}