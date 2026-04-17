
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }
int *id3(int *r) { return id2(r); }
int *id4(int *s) { return id3(s); }
int *id5(int *t) { return id4(t); }

int main() {
  int x = 42;

  int *xx = id5(&x);
  int *yy = id5(&x);

  return *xx;
}
