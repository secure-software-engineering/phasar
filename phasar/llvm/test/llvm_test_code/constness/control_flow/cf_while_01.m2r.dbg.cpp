/* k | %1 (ID: 0) | mem2reg */
int main() {
  // moved to virtual register
  int i = 0;
  int *k = new int(42);
  while (i > 0) {
    *k = 10;
  }
  return 0;
}
