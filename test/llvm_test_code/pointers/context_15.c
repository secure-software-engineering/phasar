
// The spec-mesa end() pattern: one helper called from two sites, dispatching
// through a function-pointer field of its parameter.  Context-insensitively
// both call sites merge into end's formals, so each dispatch sees both
// targets and both call results alias.
struct Ops {
  void *(*Get)(void *);
};

static void *first(void *P) { return P; }
static void *second(void *P) { return P; }

static const struct Ops O1 = {&first};
static const struct Ops O2 = {&second};

static void *end(const struct Ops *O, void *P) { return O->Get(P); }

int main() {
  int x = 42;
  int y = 43;

  void *xx = end(&O1, &x);
  void *yy = end(&O2, &y);

  return xx == yy;
}
