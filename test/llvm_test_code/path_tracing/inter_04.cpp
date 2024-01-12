extern int someFunc(int L);

int increment(int I) { return ++I; }

int modify(int I) {
  if (I < 90) {
    return someFunc(I);
  } else {
    return increment(I);
  }
  return --I;
}

int main() {
  int I = 42;
  int J = I;
  int K = increment(J);
  int L = modify(K);
  return L;
}
