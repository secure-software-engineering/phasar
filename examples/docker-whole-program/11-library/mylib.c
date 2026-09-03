/* A LIBRARY with no main(). Analyzed with -E __ALL__ so every function
   definition is treated as an entry point. 'unsafe' has an uninit-use bug. */
int add(int a, int b) { return a + b; }

int unsafe(int flag) {
  int v;
  if (flag) {
    v = 1;
  }
  return v; /* uninitialized when flag == 0 */
}
