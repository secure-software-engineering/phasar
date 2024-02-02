unsigned recursion(unsigned i) {
  if (i > 0) {
    return recursion(i - 1);
  }
  return i;
}

int main(int Argc, char **Argv) {
  Argc = recursion(5);
  return 0;
}
