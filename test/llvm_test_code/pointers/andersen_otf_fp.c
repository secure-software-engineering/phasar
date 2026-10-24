// On-the-fly function-pointer resolution:
// id is stored into fp and then called indirectly.  The OTF fixpoint must
// discover id as a callee and propagate the alias between its formal
// parameter and its return value.
static int *id(int *x) { return x; }

int main() {
  int a;
  int *p = &a;
  int *(*fp)(int *) = id;
  int *q = fp(p);
  (void)q;
  return 0;
}
