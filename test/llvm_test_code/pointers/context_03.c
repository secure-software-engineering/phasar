
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }

int main() {
  int x = 42;

  int *xx = id2(&x);
  int *yy = id2(&x);

  return *xx;
}
