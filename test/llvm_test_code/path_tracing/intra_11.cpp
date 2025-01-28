int main(int Argc, char **Argv) {
  int C, A, B;
  int I = 5;
  do {
    C = A + B;
    I++;
  } while ((I < 6 ? I : I + 1) < 12);
  return C;
}
