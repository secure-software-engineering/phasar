/* c | %1 (ID: 0) | mem2reg */
int main() {
  int *c = new int(0);
  for (int i = 1; i <= 10; ++i) {
    *c += i * i;
  }
  return 0;
}
