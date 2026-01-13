
int *argretq(int *p, int *q, int *r) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx1 = argretq(&x, &x, &x);
  int *xx2 = argretq(&x, &x, &y);
  int *xx3 = argretq(&y, &x, &y);
  int *xx4 = argretq(&y, &x, &x);
  int *yy1 = argretq(&x, &y, &x);
  int *yy2 = argretq(&x, &y, &y);
  int *yy3 = argretq(&y, &y, &y);
  int *yy4 = argretq(&y, &y, &x);

  return *xx1;
}
