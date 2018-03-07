/* immutable: i */
int main() {
  int i;
  int j = 12;
  int *pj = &j;
  *pj = 42;
  pj = &i;
  return 0;
}