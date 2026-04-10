
int main() {
  int i;
  int *p = &i;
  int **q = &p;
  **q = 13;
}
