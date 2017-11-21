#include <stdio.h>
#include <stdlib.h>
#include "src1.h"
#include "src2.h"


int main()
{
	int i = function(42);
	int j = function_mult(42);
	printf("result is: %d\n", i);
	printf("result is: %d\n", j);
	return 0;
}
