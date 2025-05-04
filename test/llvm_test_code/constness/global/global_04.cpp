/* g, i | @g, %1 (ID: 0, 4) | mem2reg */
int *g;

void foo() { *g = 99; }

int main() {
  int *i = new int(42);
  g = i;
  foo();
  return 0;
}
