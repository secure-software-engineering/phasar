
int *id1(int *p) { return p; }
int *id2(int *q) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx = id1(&x);
  int *yy = id2(&y);

  return *xx;
}
