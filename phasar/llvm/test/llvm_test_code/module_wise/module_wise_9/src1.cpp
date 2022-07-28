#include "src1.h"

void Concrete::foo(int &i) { i *= 2; }

Abstract *give_me() { return new OtherConcrete; }
