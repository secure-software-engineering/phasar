unsigned fac(unsigned i) {
  if (i == 0) {
    return 1;
  }
  return i * fac(i - 1);
}

int main() {
  unsigned a = fac(5);
  return 0;
}
