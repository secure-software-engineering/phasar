/* immutable: p */
int main() {
  int i = 12;
  int *pi = &i;
  *pi = 10;
  return 0;
}