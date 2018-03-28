/* second 'new' | %3 (ID: 3) | mem2reg! */
int main() {
  // removed p due to mem2reg
  int *p = new int(42);
  p = new int(99);
  *p = 20;
  return 0;
}
