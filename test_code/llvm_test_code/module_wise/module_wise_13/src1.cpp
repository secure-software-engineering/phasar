#include "src1.h"

<<<<<<< HEAD
int A::id(int i) {
	return i;
}


int B::id(int i) {
	return 0;
}
=======
int A::foo(int &i) {
	return i+10;
}

void A::bar(double &d) {
	double a = 20.12 + d;
}
>>>>>>> const-analysis
