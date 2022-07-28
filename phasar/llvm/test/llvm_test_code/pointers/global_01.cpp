int *g = new int(17);

void foo(int *p) { *p = 42; }

int main() {
  int *i = g;
  foo(i);
  return 0;
}
