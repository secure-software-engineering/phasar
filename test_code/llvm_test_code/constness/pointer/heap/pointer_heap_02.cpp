/* immutable: p,new[]  */
int main() {
  int *p = new int[10];
  delete[] p;
  return 0;
}