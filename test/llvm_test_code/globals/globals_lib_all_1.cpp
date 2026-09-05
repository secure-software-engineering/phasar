namespace {
struct Foo {
  int Val;
  Foo() : Val(1) {}
};
} // namespace

Foo Global;

namespace {
int helper() { return Global.Val; }
} // namespace

int apiOne() { return helper(); }

int apiTwo() { return helper() + 1; }
