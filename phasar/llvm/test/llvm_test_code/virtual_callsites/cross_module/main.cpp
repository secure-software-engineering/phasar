#include "base.h"
#include "derived.h"
#include "utils.h"

int main() {
  derived d;
  callFunction(d);
  base b;
  callFunction(b);
  return 0;
}
