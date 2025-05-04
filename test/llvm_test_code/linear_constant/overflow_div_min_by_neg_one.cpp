#include <cstdint>

int main() {
  int64_t i = -9223372036854775807; // std::numeric_limits<int64_t>::min() + 1
  int64_t j = i - 8;
  int64_t k = j / -1;
  return 0;
}
