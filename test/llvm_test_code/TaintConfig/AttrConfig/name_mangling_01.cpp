
void foo() {}

void foo(int A) {}

void foo([[clang::annotate("psr.source")]] int A, int B) {}

void foo(int A, int B, int C) {}

void foo(int A, int B, int C, [[clang::annotate("psr.source")]] int D) {}

int main() {
  foo();
  foo(1);
  foo(1, 2);
  foo(1, 2, 3);
  foo(1, 2, 3, 4);
  return 0;
}
