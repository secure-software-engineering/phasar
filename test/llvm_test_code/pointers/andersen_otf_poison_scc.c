#include <stdlib.h>

// The disqualifying store in pong() records its pointer node
// unresolved.  LCD collapses that node into the ping/pong SCC, and the
// re-check reads the cleared non-representative, so O is never poisoned and
// call_fn stays wrongly precise at {real_fn}.
struct Ops {
  void (*Fn)(void);
};

void real_fn(void) {}
void other_fn(void) {}

static void (*Hook)(struct Ops *);

struct Ops *ping(struct Ops *o, void (*f)(void));

// pong's formal is created after ping's, so it becomes the non-rep.
struct Ops *pong(struct Ops *o, void (*f)(void)) {
  o->Fn = f;
  return ping(o, f);
}

struct Ops *ping(struct Ops *o, void (*f)(void)) { return pong(o, f); }

void deliver(struct Ops *o) { ping(o, other_fn); }

void call_fn(struct Ops *o) { (*o->Fn)(); }

int main() {
  struct Ops *O = (struct Ops *)malloc(sizeof(struct Ops));
  O->Fn = real_fn;
  Hook = deliver;

  // Drive one wave through the ping/pong cycle so LCD collapses it while O
  // is still absent from the formals' points-to sets.
  struct Ops *Seed = (struct Ops *)malloc(sizeof(struct Ops));
  ping(Seed, other_fn);

  // Indirect through a load of Hook: deliver(O) is only connected in a later
  // round, so O reaches the collapsed formals after the merge.
  (*Hook)(O);

  call_fn(O);
  return 0;
}
