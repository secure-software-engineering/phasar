extern bool cond;
int main() {
  int j = 10;
  int i = 42;
  if (cond) {
    i = j + 32;
  }
  return 0;
}
