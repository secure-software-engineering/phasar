// Soundness test: close_stdout is passed as a fn-ptr arg to the external
// register_callback (a declaration).  At Soundy/Sound, the solver must
// treat close_stdout as a reachable entry point and analyse its body,
// discovering flush_impl as a callee.  At Unsound neither should appear.
void flush_impl(void) {}

void close_stdout(void) { flush_impl(); }

// External: only a declaration, body not available in this module.
void register_callback(void (*f)(void));

int main(void) {
  register_callback(close_stdout);
  return 0;
}
