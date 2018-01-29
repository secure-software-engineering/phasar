/* mutable: - */
int main() {
  int *i = new int[10];
  delete[] i;
  return 0;
}