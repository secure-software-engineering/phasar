#include "src2.h"

Abstract *other() {
	OtherConcrete o;
	int i = 2;
	o.foo(i);
	return new OtherConcrete;
}