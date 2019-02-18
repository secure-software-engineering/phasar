// Handle call function from inside and ouside of a class

class Foo{public : void f(){}};

void f() {}

int main() {
  Foo foo;
  foo.f();
  f();
}