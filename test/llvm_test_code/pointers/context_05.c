
int *buzz(int *s) { return s; }
int *baz(int *r) { return buzz(r); }
int *bar(int *q) { return baz(q); }
int *foo(int *p) { return bar(p); }

int main() {
  int x = 42;
  int y = 43;

  int *xx = foo(&x);
  int *yy = foo(&y);

  return *xx;
}
