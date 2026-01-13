
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }
int *id3(int *r) { return id2(r); }
int *id4(int *s) { return id3(s); }

int main() {
  int x = 42;

  int *xx = id4(&x);
  int *yy = id4(&x);

  return *xx;
}
