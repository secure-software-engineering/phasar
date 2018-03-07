/* immutable: - */
#include <array>
int main() {
  std::array<int,3> arr = {10,20,30};
  arr.at(0) = 42;
  return 0;
}