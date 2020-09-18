void bar(int b) {}

void foo(int a) {
  bar(a);
}

int main() {
  int i;
  foo(2);
  return 0;
}