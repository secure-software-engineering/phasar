// Handle function call to a class from another class

class Foo1 {
public:
  void f() {}
};

class Foo2 {
public:
  void f() {
    Foo1 foo1;
    foo1.f();
  }
};

int main() {
  Foo2 foo2;
  foo2.f();
}