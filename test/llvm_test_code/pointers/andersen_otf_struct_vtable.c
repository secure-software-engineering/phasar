// Test: hand-rolled C vtable via const struct global.
// ops->write(...) must resolve precisely to myWrite, not myRead.
// Field-insensitive analysis would add both; the struct-vtable path
// reads the initializer at the specific field index.

static int myRead(void *ctx) { return 0; }
static int myWrite(void *ctx, int v) { return v; }

struct Ops { int (*read)(void *); int (*write)(void *, int); };

static const struct Ops myOps = { myRead, myWrite };

int dispatch(const struct Ops *ops, void *ctx, int v) {
  return ops->write(ctx, v);
}

int main(void) { return dispatch(&myOps, 0, 42); }
