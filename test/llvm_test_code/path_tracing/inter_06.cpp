int add(int A, int B) { return A + B; }

int main(int Argc, char **Argv) {
  int A = 42;
  int B = A++;
  int (*F)(int, int) = &add;
  int C = F(A, B);
  return C;
}
