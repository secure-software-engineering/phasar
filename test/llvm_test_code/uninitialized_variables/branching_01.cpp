bool somePredicate();

int main() {
  int a;
  int b = 42;
  if (somePredicate()) {
    b = 13;
  } else {
    a = b;
  }
  return b;
}
