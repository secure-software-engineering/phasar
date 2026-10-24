extern void __kmpc_fork_call(void *loc, int argc,
                             void (*microtask)(int *, int *, ...), ...);

void microtask(int *gtid, int *bound, int *payload) {
  (void)gtid;
  (void)bound;
  (void)payload;
}

int main() {
  int payload = 0;
  __kmpc_fork_call(0, 1, (void (*)(int *, int *, ...))microtask, &payload);
  return 0;
}
