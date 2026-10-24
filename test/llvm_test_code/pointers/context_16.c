
// A dispatch table filled in by a shared helper, called from two contexts.
// Context-insensitively the two tables are one object, so both call sites see
// both callees (FnPtrFieldWrites keys on the allocation site alone).
#include <stdlib.h>

struct Table {
  void (*Fn)(void);
};

static void red(void) {}
static void blue(void) {}

static struct Table *make(void (*Fn)(void)) {
  struct Table *T = malloc(sizeof(struct Table));
  T->Fn = Fn;
  return T;
}

int main() {
  struct Table *A = make(&red);
  struct Table *B = make(&blue);

  A->Fn();
  B->Fn();

  return 0;
}
