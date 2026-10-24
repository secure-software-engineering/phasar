// Two virtual methods in the same vtable.
// call_getX (slot 0) must alias @x; call_getY (slot 1) must alias @y.
// With imprecise (all-slots) vtable handling both rets would alias both
// globals; the slot-specific path must keep them separate.
struct B {
  virtual int *getX();
  virtual int *getY();
};

int x, y;
int *B::getX() { return &x; }
int *B::getY() { return &y; }

static int *call_getX(B *b) { return b->getX(); }
static int *call_getY(B *b) { return b->getY(); }

int main() {
  B b;
  int *px = call_getX(&b);
  int *py = call_getY(&b);
  return 0;
}
