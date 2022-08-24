#include "source_1.h"
#include "source_2.h"

const int src1_const = 42;

int src1_nonconst = 13;

int one = 1;

int increment(int a) { return a + one; }

int add_and_div(int a, int b, int c) { return a + divide(b, c); }
