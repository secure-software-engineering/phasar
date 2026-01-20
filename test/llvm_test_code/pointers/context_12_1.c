
int *argretq(int *p, int *q) { return q; }

int main() {
  int x = 42;
  int y = 43;

  int *xx1 = argretq(&y, &x);
  int *yy1 = argretq(&x, &y);

  return *xx1;
}
