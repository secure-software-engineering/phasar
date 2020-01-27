unsigned foo(unsigned i) {
  if (i == 0) {
    return 1;
  }
  return foo(--i);
}

int main() {
  unsigned a = foo(5);
  return 0;
}