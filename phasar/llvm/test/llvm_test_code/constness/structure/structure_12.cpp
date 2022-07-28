/* immutable: c */
class Base {
  virtual int foo() { return 1; }
};

class Child : Base {
  int foo() override { return 2; }
};

int main() {
  Child c;
  return 0;
}
