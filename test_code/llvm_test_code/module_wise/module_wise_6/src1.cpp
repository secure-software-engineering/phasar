#include "src1.h"

void foo(int &i) {
	i = 42;
	boo(i);
}

void boo(int &i) {
 i = 13;
}