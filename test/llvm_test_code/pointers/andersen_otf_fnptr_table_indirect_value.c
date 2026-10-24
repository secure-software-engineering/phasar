#include <stdlib.h>

// A later write stores a function-pointer *variable*, not a literal
// function: call_fn must fall back to {real_fn, alt_fn}, not {real_fn}.
struct Ops {
  void (*Fn)(int *, int *);
};

void real_fn(int *p, int *q) {}
void alt_fn(int *p, int *q) {}

void init_direct(struct Ops *o) { o->Fn = real_fn; }
void init_indirect(struct Ops *o, void (*f)(int *, int *)) { o->Fn = f; }
void call_fn(struct Ops *o, int *p, int *q) { (*o->Fn)(p, q); }

int main() {
  struct Ops *O = (struct Ops *)malloc(sizeof(struct Ops));
  init_direct(O);
  init_indirect(O, alt_fn);
  int X, Y;
  call_fn(O, &X, &Y);
  return 0;
}
