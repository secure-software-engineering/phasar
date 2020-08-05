int main(int argc, char *argv[]) {
  int i = 3;
  char c = 0;
#ifdef A
  if (i < argc) {
    c = argv[i];
  } else {
    c = 3;
  }
#else
  if (i < argc - 1)
    c = argv[i + 1];
  else {
    c = 4;
  }
#endif
  return c;
}