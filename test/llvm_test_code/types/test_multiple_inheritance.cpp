// Test for multiple inheritance with virtual functions
class Base1 {
public:
  virtual ~Base1() = default;
  virtual void method1() {}
  int data1;
};

class Base2 {
public:
  virtual ~Base2() = default;
  virtual void method2() {}
  int data2;
};

class Derived : public Base1, public Base2 {
public:
  void method1() override {}
  void method2() override {}
  int derivedData;
};

void test_multiple_inheritance(Derived *d) {
  Base1 *b1 = d;
  Base2 *b2 = d;

  // Virtual calls through different base pointers
  b1->method1(); // Should resolve to Derived::method1
  b2->method2(); // Should resolve to Derived::method2

  // Direct access to derived object
  d->method1();
  d->method2();
}

int main() {
  Derived d;
  test_multiple_inheritance(&d);
  return 0;
}
