struct B {
  int x;
};

void foo(B ***bppp) {
  B **bpp = *bppp;
  B *bp = *bpp;
  int val = bp->x;
  (void)val;
}
