// Demonstrates Bug 2: outer fixpoint exits when FunctionWorklist is empty
// even though checkUnresolvedFPCalls just grew pts for a call site that was
// already examined earlier in the same pass.
//
// Processing order (LIFO FunctionWorklist; main pushes D, A, B):
//   pop B → call2 (g_fp2()) deferred, pts={}.
//   pop A → call1 (g_fp1(get_y)) deferred, pts={} (D not yet run).
//   pop D → relay processed (g_fp2=get_x), g_fp1=relay set.
//   After D, propagation: pts(g_fp2_load)={get_x}, pts(g_fp1_load)={relay}.
//
// checkUnresolvedFPCalls: [call2, call1]
//   call2: pts(g_fp2_load)={get_x} → connects get_x. ret(B) gets x.
//   call1: pts(g_fp1_load)={relay} → connects relay with arg get_y
//          → relay already processed → propagate → g_fp2 gains get_y.
//   FunctionWorklist still empty → outer loop exits. call2 re-check skipped.
//
// Expected (sound): ret(B) must alias both x and y.

int x, y;

static int *get_x(void) { return &x; }
static int *get_y(void) { return &y; }

static int *(*g_fp2)(void);
static void (*g_fp1)(int *(*)(void));

static void relay(int *(*cb)(void)) { g_fp2 = cb; }

// Processed first (B pushed last by main).
// g_fp2 is still unset, so call2 deferred with pts={}.
static int *B(void) { return g_fp2(); }

// Processed second (A pushed second by main).
// g_fp1 is still unset (D not yet run), so call1 deferred with pts={}.
static void A(void) { g_fp1(get_y); }

// Processed third (D pushed first by main).
// Ensures relay, get_x, get_y are all processed before checkUnresolved runs.
static void D(void) {
  get_x();
  get_y();
  relay(get_x);
  g_fp1 = relay;
}

int main(void) {
  D(); // pushed first → bottom of stack → processed third
  A(); // pushed second → processed second
  B(); // pushed third → top of stack → processed first
  return 0;
}
