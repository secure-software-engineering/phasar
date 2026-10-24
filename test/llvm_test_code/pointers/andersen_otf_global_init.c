// Global pointer @p is initialised to &@x.
// Loading from @p must yield a pointer that aliases @x (Bug 2 soundness).
int x = 0;
int *p = &x;

int main() {
  int *q = p;
  return 0;
}
