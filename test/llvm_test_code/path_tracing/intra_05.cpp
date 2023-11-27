int main(int Argc, char **Argv) {
  int I = 2000;
  int J = 4000;
  if (Argc > 1) {
    I = 1;
    J = 4;
  } else {
    I = 2;
    J = 4;
  }
  return J;
}
