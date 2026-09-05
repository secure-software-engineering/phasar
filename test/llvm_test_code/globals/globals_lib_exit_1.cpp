#include <cstdlib>

namespace {
struct Foo {
  int Val;
  Foo() : Val(1) {}
  ~Foo() { Val = 0; }
};
} // namespace

Foo Global;

void bail() {
  if (Global.Val) {
    std::exit(1);
  }
}
