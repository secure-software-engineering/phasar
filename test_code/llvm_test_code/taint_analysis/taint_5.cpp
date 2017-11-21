#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	char *buffer;
	FILE *F;
	fread(buffer, 10, 10, F);
	return 0;
}