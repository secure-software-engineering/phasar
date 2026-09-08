#include <cstddef>

int process(const char *Buf, size_t Len) {
  int Sum = 0;
  for (size_t Idx = 0; Idx < Len; ++Idx) {
    Sum += Buf[Idx];
  }
  return Sum;
}
