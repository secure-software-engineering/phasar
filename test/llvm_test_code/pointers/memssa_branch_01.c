int main(int c) {
  int a;
  int b;
  int *p;
  if (c)
    p = &a;
  else
    p = &b;
  int *q = p;
  return 0;
}
