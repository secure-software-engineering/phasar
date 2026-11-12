#include <stdlib.h>

static int cmp(const void *A, const void *B) {
  return *(const int *)A - *(const int *)B;
}

int main(void) {
  int Arr[] = {3, 1, 2};
  qsort(Arr, 3, sizeof(int), cmp);
  return Arr[0];
}
