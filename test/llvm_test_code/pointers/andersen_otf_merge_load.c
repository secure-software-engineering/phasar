// h->f->h cycle; h returns *p (the load result).
// After both h(&px) and h(&py), h's return value must alias x and y.
static int *f(int **p);

static int *h(int **p) {
  f(p);
  return *p;
}

static int *f(int **p) {
  return h(p);
}

int main() {
  int x = 0;
  int y = 0;
  int *px = &x;
  int *py = &y;
  h(&px);
  h(&py);
  return 0;
}
