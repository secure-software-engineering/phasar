extern void bar(int x, int y);

void foo(int x, int y) { bar(x, y); }

int main() {
  foo(1, 2);
  foo(2, 3);
}
