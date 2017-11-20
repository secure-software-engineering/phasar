#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	FILE *F;// = fopen(argv[0], "r");
//	char *buffer = (char*) malloc(10);
char *buffer;
fread(buffer, 10, 10, F);
	fwrite(buffer, 10, 10, F);
//	fclose(F);
//	free(buffer);
	return 0;
}