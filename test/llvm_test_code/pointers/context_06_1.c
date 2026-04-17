
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }
int *id3(int *r) { return id2(r); }
int *id4(int *s) { return id3(s); }
int *id5(int *t) { return id4(t); }

int main() {
  int x = 4;
  int y = 2;

  int *xx1 = id5(&x);
  int *xx2 = id5(&x);
  int *yy1 = id5(&y);
  int *yy2 = id5(&y);

  return *yy1;
}
