/* a | %1 (ID: 1) | mem2reg*/
#include <array>
int main() {
  std::array<int, 3> a = {10, 20, 30};
  a[0] = 42;
  return 0;
}
