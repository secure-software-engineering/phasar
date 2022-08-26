int main(int Argc, char **Argv) {
  int i;
  switch (Argc) {
  case 1:
    i = 10;
    break;
  case 2:
  case 3:
    i = 20;
    break;
  case 4:
    i = 30;
  default:
    i = -1;
  }
  return i;
}
