int main() {
  int i;
  int *p = &i;
  // p still counts as uninitialized at this point
  // thus dereferencing counts as uninitialized use
  *p = 42;
  return 0;
}
