int* foo() {
  return new int(42);
}

int main() {
  int *i = foo();
  return 0;
}