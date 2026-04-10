
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }
int *id3(int *r) { return id2(r); }
int *id4(int *s) { return id3(s); }

int main() {
  int x = 4;
  int y = 2;

  int *xx1 = id4(&x);
  int *xx2 = id4(&x);
  int *yy1 = id4(&y);
  int *yy2 = id4(&y);

  return *xx2;
}
