int add(int A, int B) {
  int C;
  for (int I = 0; I < 12; I++) {
    C = A + B;
  }
  return C;
}

int sub(int A, int B) {
  int C;
  int I = 5;
  while (I < 12) {
    C = A + B;
    I++;
  }
  return C;
}

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
