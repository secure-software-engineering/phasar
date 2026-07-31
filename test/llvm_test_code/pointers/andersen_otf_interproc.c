// Direct interprocedural alias propagation:
// retptr returns its argument, so the formal param and the return value
// share the same points-to set.
static int *retptr(int *x) { return x; }

int main() {
  int a;
  int *p = &a;
  int *q = retptr(p);
  (void)q;
  return 0;
}
