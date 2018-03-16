/* immutable: p1,p2 */
int main() {
  int i = 13;
  int *p1 = &i;
  int *p2 = p1;
  *p2 = 42;
  return 0;
}