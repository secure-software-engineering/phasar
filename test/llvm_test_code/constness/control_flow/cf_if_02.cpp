/* - | - | mem2reg */
extern bool cond;
int main() {
  int *i;
  if (cond) {
    // moved to virtual register
    int j = 20;
    j++;
  } else {
    i = new int(30);
  }
  return 0;
}
