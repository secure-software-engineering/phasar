int main(int argc, char *argv[]) {
  int i = 42;
  int j = 24;
  if (argc > 1)
    i = i + 1;
  else
    i = j + 1;
  return i;
}
