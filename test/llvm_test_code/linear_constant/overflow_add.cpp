#include <cstdint>

int main() {
  int64_t i = 9223372036854775806; // std::numeric_limits<int64_t>::max() - 1
  int64_t j = i + 8;
  return 0;
}
