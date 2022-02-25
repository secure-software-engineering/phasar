/* a | %1 (ID: 1) | mem2reg*/
int main() {
  int a[2] = {42, 99};
  // moved to register due to mem2reg
  int *i = a + 1;
  *i = 17;
  return 0;
}
