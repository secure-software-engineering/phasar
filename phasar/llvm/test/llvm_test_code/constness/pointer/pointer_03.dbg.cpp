/* j, pj | %3, %4 (ID: 2, 3) */
int main() {
  int i;
  int j = 12;
  int *pj = &j;
  *pj = 42;
  pj = &i;
  return 0;
}
