
#ifdef USE_LONG_INT
typedef unsigned long long size_t;
#else
typedef unsigned int size_t;
#endif

extern void *malloc(size_t);
extern void free(void *);

int main() {
  int *p = malloc(4);
  *p = sizeof(size_t);
  int *q = p;
  *p++;
  *q = 42;
  int i = *p;
  int j = *q;
  free(q);
  return i;
}