extern void __kmpc_fork_teams(void *loc, int argc,
                              void (*microtask)(int *, int *, ...), ...);

void team_microtask(int *gtid, int *bound, int *payload) {
  (void)gtid;
  (void)bound;
  (void)payload;
}

int main() {
  int payload = 0;
  __kmpc_fork_teams(0, 1, (void (*)(int *, int *, ...))team_microtask,
                    &payload);
  return 0;
}
