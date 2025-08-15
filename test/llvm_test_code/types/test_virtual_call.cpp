class B {
public:
  virtual void foo();
};

void B::foo() {
  // empty implementation
}

void test_virtual_call(B *b) { //
  b->foo();
}
