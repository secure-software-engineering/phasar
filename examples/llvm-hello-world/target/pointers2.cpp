#include <cstdlib>

int main(int argc, char **argv) {
  constexpr int N = 10;
  int *mem = static_cast<int *>(malloc(N * sizeof(int)));
  for (int i = 0; i < N; ++i) {
    mem[i] = 42;
  }
  free(mem);
  return 0;
}
