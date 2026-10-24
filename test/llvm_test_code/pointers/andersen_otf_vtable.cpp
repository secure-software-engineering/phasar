// Virtual dispatch via a pointer forces the vtable lookup path.
// call_get's return value must alias @x (returned by A::get).
struct A {
  virtual int *get();
};

int x;
int *A::get() { return &x; }

static int *call_get(A *a) { return a->get(); }

int main() {
  A a;
  int *p = call_get(&a);
  return 0;
}
