
int *argretq(int *p, int *q, int *r) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx1 = argretq(&x, &x, &x);
  int *yy1 = argretq(&y, &y, &y);

  return *yy1;
}
