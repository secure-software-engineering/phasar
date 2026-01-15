
int *id1(int *p) { return p; }
int *id2(int *q) { return id1(q); }
int *id3(int *r) { return id2(r); }

int main() {
  int x = 42;

  int *xx1 = id3(&x);
  int *xx2 = id3(&x);

  return *xx1;
}
