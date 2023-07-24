unsigned factorial(unsigned n) {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

int id(int i) { return i; }

int sum(int i, int j) { return i + j; }

int main() {
  int i = 7;
  int j = factorial(i);
  int k = id(j);
  k += sum(i, j);
  return k;
}
