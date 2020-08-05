int main(int argc, char *argv[]) {
  int i = 4;
  if (i < argc)
    i = i + 3;
#ifdef A
  else

    i--;
#endif
  int j = i
#ifdef B
                  >
#else
                  <
#endif
                  5
              ? 2
              : 556;
  return j;
}