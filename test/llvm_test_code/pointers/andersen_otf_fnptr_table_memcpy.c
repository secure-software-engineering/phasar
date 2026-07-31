#include <stdlib.h>

// Minimized spec-mesa pattern: H->A and H->B alias H (field-insensitively),
// only H->B is initialized, then H->A = H->B (an llvm.memcpy). Each field's
// call through H->A must still resolve to only its own function.
struct Ops {
  void (*Foo)(int *, int *);
  void (*Bar)(int *, int *);
};

void foo_impl(int *p, int *q) {}
void bar_impl(int *p, int *q) {}

void init_ops(struct Ops *o) {
  o->Foo = foo_impl;
  o->Bar = bar_impl;
}

void call_foo(struct Ops *o, int *p, int *q) { (*o->Foo)(p, q); }
void call_bar(struct Ops *o, int *p, int *q) { (*o->Bar)(p, q); }

struct Holder {
  struct Ops A;
  struct Ops B;
};

int main() {
  struct Holder *H = (struct Holder *)malloc(sizeof(struct Holder));
  init_ops(&H->B);
  H->A = H->B;
  int W, X, Y, Z;
  call_foo(&H->A, &W, &X);
  call_bar(&H->A, &Y, &Z);
  return 0;
}
