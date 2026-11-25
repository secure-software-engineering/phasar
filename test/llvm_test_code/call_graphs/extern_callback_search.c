#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <search.h>
#include <stdlib.h>

static int cmpB(const void *A, const void *B) {
  return *(const int *)A - *(const int *)B;
}

static int cmpL(const void *A, const void *B) {
  return *(const int *)A - *(const int *)B;
}

static int cmpR(const void *A, const void *B, void *Arg) {
  return *(const int *)A - *(const int *)B + *(int *)Arg;
}

int main(void) {
  int Arr[] = {1, 2, 3};
  int Key = 2;
  int Ctx = 0;
  size_t N = 3;
  int *B = bsearch(&Key, Arr, 3, sizeof(int), cmpB);
  int *L = lfind(&Key, Arr, &N, sizeof(int), cmpL);
  qsort_r(Arr, 3, sizeof(int), cmpR, &Ctx);
  return *B + *L;
}
