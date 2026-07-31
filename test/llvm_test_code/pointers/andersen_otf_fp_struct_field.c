// Test: function pointer stored in a struct field via an initializer function,
// then retrieved and called indirectly. Mirrors the obstack chunkfun pattern.
// Expected: the indirect call in do_call() must have target() as a callee.

struct Ctx {
  void *(*fn)(void *);
};

static void *target(void *arg) { return arg; }

static void init_ctx(struct Ctx *ctx, void *(*fn)(void *)) { ctx->fn = fn; }

static void *do_call(struct Ctx *ctx, void *arg) {
  return ctx->fn(arg); // indirect call via struct field
}

int main(void) {
  struct Ctx ctx;
  init_ctx(&ctx, target);
  do_call(&ctx, (void *)0);
  return 0;
}
