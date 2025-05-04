extern int rand();

int foo(int x) {
  while (rand()) {
    x++;
  }
  return x;
}

int main() {
  int a = 3;
  int b = foo(a);
  return b;
}
