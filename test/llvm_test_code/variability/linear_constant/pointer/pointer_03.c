typedef
#ifdef X64
    long long
#else
    int
#endif
        ptrdiff_t;

ptrdiff_t compute(int x) {
  int *y = &x;
  int **z = &y;
  ptrdiff_t u = z;

#ifdef A
  return u;
#else
  return **z + 2;
#endif
}

int main() {
  int i = 42;
  ptrdiff_t j = compute(i);
  return j + 1;
}