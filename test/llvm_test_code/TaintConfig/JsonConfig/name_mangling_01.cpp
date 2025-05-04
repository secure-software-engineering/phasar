
void foo() {}

void foo(int A) {}

void foo(int A, int B) {}

void foo(int A, int B, int C) {}

void foo(int A, int B, int C, int D) {}

int main() {
  foo();
  foo(1);
  foo(1, 2);
  foo(1, 2, 3);
  foo(1, 2, 3, 4);
  return 0;
}
