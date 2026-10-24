#include <stdlib.h>

// Malloc'd dispatch table: each field's indirect call must resolve only to
// its own function, not both.
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

int main() {
  struct Ops *O = (struct Ops *)malloc(sizeof(struct Ops));
  init_ops(O);
  int X, Y;
  call_foo(O, &X, &Y);
  call_bar(O, &Y, &X);
  return 0;
}
