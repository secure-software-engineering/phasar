
#include <cstddef>
#include <cstring>
#include <utility>

struct Foo {
  int X = 0;

  Foo() noexcept = default;
  Foo(const Foo &Other) noexcept : X(Other.X) {}
};

Foo createFoo() { return {}; }

int main() {
  Foo F;
  F = createFoo();
  return F.X;
}
