#include <cstdio>

// struct Foo {
//   int x;
//   Foo(int x) noexcept : x(x) {}
//   ~Foo() noexcept {
//     ++x;
//     printf("x: %d\n", x);
//     x = 0;
//   }
// };

// Foo createInitialFoo() noexcept {
//   int x = 42;
//   return Foo(x);
// }

// Foo foo = createInitialFoo();

// int main() { printf("x: %d\n", foo.x); }

// struct Foo is not an integer, so the LCA ignores it...
// => Create an artificial example that models the same behaviour and memory
//    layout as above

using Foo = int;

Foo createFoo() noexcept { return 42; }

void Foo_dtor(Foo &_this) noexcept {
  ++_this;
  printf("x: %d\n", _this);
  _this = 0;
}

extern "C" int __cxa_atexit(void (*dtor)(void *), void *, void *);

Foo foo;

__attribute__((constructor)) void makeGlobalFoo() {
  foo = createFoo();
  __cxa_atexit((void (*)(void *)) & Foo_dtor, &foo, nullptr);
}

int main() { printf("x: %d\n", foo); }
