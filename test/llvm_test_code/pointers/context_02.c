
int *id1(int *p) { return p; }
int *id2(int *q) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx1 = id1(&x);
  int *xx2 = id1(&x);
  int *yy1 = id2(&y);
  int *yy2 = id2(&y);

  return *xx1;
}
