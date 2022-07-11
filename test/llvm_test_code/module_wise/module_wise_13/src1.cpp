#include "src1.h"

int A::id(int i) { return i; }

int A::foo(int &i) { return i + 10; }

void A::bar(double &d) { double a = 20.12 + d; }
