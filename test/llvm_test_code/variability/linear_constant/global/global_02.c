
int a = 42;
#ifdef A
int b = 13;
#endif

int main() {
  int x = a + 200;
#ifdef A
  x += b;
#endif
  return 0;
}
