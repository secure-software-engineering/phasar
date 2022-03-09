struct base {
  virtual int foo() { return 1; }
};

struct child : base {
  virtual int foo() { return 10; }
};

int main() {
  child c;
  c.foo();
  return 0;
}
