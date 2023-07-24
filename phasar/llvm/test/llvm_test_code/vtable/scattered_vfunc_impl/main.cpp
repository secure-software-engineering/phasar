#include "base.h"
#include "child.h"

struct nonvirtual {
  int i;
};

int main() {
  nonvirtual n;
  n.i = 100;
  base *c = new child;
  c->foo();
  delete c;
  return 0;
}
