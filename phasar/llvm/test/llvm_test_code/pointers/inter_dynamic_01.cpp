
#include <cstdlib>

void init(int *p) { *p = 13; }

int main() {
  int *p = static_cast<int *>(malloc(sizeof(int)));
  init(p);
  free(p);
}
