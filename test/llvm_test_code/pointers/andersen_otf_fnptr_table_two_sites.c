#include <stdlib.h>

// Two malloc'd tables with different assignments; each dispatcher's own
// call site must resolve only to its own object's function.
struct Ops {
  void (*Fn)(int *, int *);
};

void alpha(int *p, int *q) {}
void beta(int *p, int *q) {}

void init_alpha(struct Ops *o) { o->Fn = alpha; }
void init_beta(struct Ops *o) { o->Fn = beta; }

void call_via_1(struct Ops *o, int *p, int *q) { (*o->Fn)(p, q); }
void call_via_2(struct Ops *o, int *p, int *q) { (*o->Fn)(p, q); }

int main() {
  struct Ops *O1 = (struct Ops *)malloc(sizeof(struct Ops));
  struct Ops *O2 = (struct Ops *)malloc(sizeof(struct Ops));
  init_alpha(O1);
  init_beta(O2);
  int X, Y;
  call_via_1(O1, &X, &Y);
  call_via_2(O2, &Y, &X);
  return 0;
}
