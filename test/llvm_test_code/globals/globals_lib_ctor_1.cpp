namespace {
struct Foo {
  int Val;
  Foo() : Val(42) {}
  ~Foo() { Val = 0; }
};
} // namespace

Foo Global;

int getVal() { return Global.Val; }
