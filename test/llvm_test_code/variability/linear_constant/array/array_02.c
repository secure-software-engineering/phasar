
int main() {
  int array[10];
#ifdef A
  array[0] = 13;
#else
  array[0] = 42;
#endif
  return 0;
}
