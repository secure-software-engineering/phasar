/* i | %2 (ID: 1) */
int main() {
  int i = 12;
  int *p = &i;
  int **pp = &p;
  **pp = 10;
  return 0;
}