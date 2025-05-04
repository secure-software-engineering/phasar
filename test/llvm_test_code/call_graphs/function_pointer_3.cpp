
int foo() { return 42; }

int bar(int i) { return 13; }

int main() {
  int (*fptr)();
  fptr = &foo;
  fptr = (int (*)()) & bar;
  int result = fptr();
  return result;
}
