/* mutable: p */
int main() {
  int i = 12;
  int j = 7;
  int *pi = &i;
  pi = &j;
  return 0;
}