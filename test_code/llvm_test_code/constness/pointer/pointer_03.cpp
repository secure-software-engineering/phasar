/* immutable: pi,ppi */
int main() {
  int i = 12;
  int *pi = &i;
  int **ppi = &pi;
  **ppi = 10;
  return 0;
}