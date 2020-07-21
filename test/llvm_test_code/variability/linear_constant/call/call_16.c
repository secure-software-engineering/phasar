typedef struct _state {
  int state;
  int i;
} state_t;

#define yield(i, x)                                                            \
  s->state = i;                                                                \
  return (x);                                                                  \
  case i:

// A simple stackless coroutine
int coro(state_t *s, int n) {
  switch (s->state) {
  case 0:
    for (s->i = 0; s->i < n; ++s->i) {
      yield(1, s->i * 2);
    }
    yield(2, n * 2);
  }
}

int main() {
  state_t s;
  int n = 3;
  // 0
  int i = coro(&s, n);
  // 2
  int j = coro(&s, n);
  // 4
  int k = coro(&s, n);
  return k;
}