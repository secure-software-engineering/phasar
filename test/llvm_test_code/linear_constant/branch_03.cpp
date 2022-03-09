extern bool cond;
int main() {
  int i = 42;
  if (cond) {
    i = 10;
  }
  i = 30;
  return 0;
}
