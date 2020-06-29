
int main() {
  int x;
#ifdef A
  x = 100;
#else
  x = 200;
#endif
  return 0;
}
