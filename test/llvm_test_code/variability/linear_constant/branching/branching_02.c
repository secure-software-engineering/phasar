
int main() {
  int x = 100;
  if (x > 0) {
#ifdef A
    x = -200;
#else
    x = 400;
#endif
  }
  return 0;
}
