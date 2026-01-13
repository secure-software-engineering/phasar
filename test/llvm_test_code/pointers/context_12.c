
int *argretq(int *p, int *q) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx1 = argretq(&x, &y);
  int *xx2 = argretq(&x, &y);
  int *yy1 = argretq(&y, &x);
  int *yy2 = argretq(&y, &x);

  return *xx1;
}
