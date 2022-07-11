/* a | %1 (ID: 2) | mem2reg*/
#include <array>
int main() {
  std::array<int, 3> a = {10, 20, 30};
  a.at(0) = 42;
  return 0;
}
