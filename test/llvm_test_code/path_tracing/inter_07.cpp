int add(int A, int B) { return A + B; }

int sub(int A, int B) { return A - B; }

int main(int Argc, char **Argv) {
  int A = 42;
  int B = A++;
  int C;
  if (Argc > 0) {
    int (*F)(int, int) = &add;
    C = F(A, B);
  } else {
    int (*F)(int, int) = &sub;
    C = F(A, B);
  }

  return C;
}
