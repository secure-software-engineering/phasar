/* i | %1 (ID: 2) | mem2reg */
void foo(int *p) { *p = 42; }

int main() {
  int i;
  foo(&i);
  foo(&i);
  foo(&i);
  return 0;
}
