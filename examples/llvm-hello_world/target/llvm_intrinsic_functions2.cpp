#include <cstdlib>
#include <cstring>

int foo() { return 42; }

int main() {
  int *i = static_cast<int *>(malloc(10 * sizeof(int)));
  memset(i, 0, 10 * sizeof(int));
  free(i);
  int *j = new int(13);
  *j = foo();
  delete j;
  return 0;
}
