
// Mutual recursion: ping and pong call each other and are both reached from
// two call sites in main.  Exercises context-string truncation (k = 1) -- the
// solver must terminate and stay sound.
static int *pong(int *P);

static int *ping(int *P) { return P ? pong(P) : P; }
static int *pong(int *P) { return P ? ping(P) : P; }

int main() {
  int x = 42;
  int y = 43;

  int *xx = ping(&x);
  int *yy = ping(&y);

  return xx == yy;
}
