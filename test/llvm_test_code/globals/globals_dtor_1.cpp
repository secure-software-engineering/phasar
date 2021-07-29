#include <cstdio>

struct Foo {
  ~Foo() { puts("Call ~Foo()\n"); }
  void foo() { puts("foo\n"); }
};

Foo foo1;

void bar() {
  static Foo foo2;
  foo2.foo();
}

int main() {
  foo1.foo();
  bar();
}