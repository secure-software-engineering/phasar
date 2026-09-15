#include <stdio.h>
#include <stdlib.h>
/* A small program with a mix of constructs so the IR statistics are
   non-trivial: globals, a heap allocation, a loop with loads/stores, and two
   functions.     */
int global_counter = 0;

static int accumulate(int *arr, int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += arr[i];
    global_counter++;
  }
  return sum;
}

int main(void) {
  int *data = (int *)malloc(sizeof(int) * 4);
  for (int i = 0; i < 4; ++i) {
    data[i] = i * i;
  }
  printf("%d (counter=%d)\n", accumulate(data, 4), global_counter);
  free(data);
  return 0;
}
