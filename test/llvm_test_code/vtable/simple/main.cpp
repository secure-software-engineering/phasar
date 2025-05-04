#include "base.h"

struct child : base {
  virtual int foo() { return 20; }
  int other() { return 30; }
};

int main() {
  child c;
  return 0;
}
