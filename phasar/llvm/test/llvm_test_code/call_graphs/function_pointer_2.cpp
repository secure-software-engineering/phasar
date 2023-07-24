
int foo() { return 42; }

int bar() { return 13; }

int main() {
  int (*fptr)() = &bar;
  int result = fptr();
  return result;
}
