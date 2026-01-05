
int *id(int *p) { return p; }

int main() {
  int x = 42;
  int y = 43;

  int *xx = id(&x);
  int *yy = id(&y);

  return *xx;
}
