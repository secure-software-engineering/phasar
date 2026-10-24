int main() {
  int a;
  int b;
  int *p = &a;
  p = &b;
  int *q = p;
  return 0;
}
