#include <stdlib.h>

// A non-constant-index write into one field poisons the whole object:
// call_fn must fall back to {real_fn, bogus}, not stay precise at {real_fn}.
struct Ops {
  void (*Fn)(int *, int *);
  void (*Extra[2])(int *, int *);
};

void real_fn(int *p, int *q) {}
void bogus(int *p, int *q) {}

void init_ops(struct Ops *o) { o->Fn = real_fn; }
void poke_dynamic(struct Ops *o, int idx, void (*f)(int *, int *)) {
  o->Extra[idx] = f;
}
void call_fn(struct Ops *o, int *p, int *q) { (*o->Fn)(p, q); }

int main() {
  struct Ops *O = (struct Ops *)malloc(sizeof(struct Ops));
  init_ops(O);
  poke_dynamic(O, 0, bogus);
  int X, Y;
  call_fn(O, &X, &Y);
  return 0;
}
