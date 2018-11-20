// handle call to basic function object

struct A {
  void operator()() {}
};

int main() {
  A a;
  a();
}
