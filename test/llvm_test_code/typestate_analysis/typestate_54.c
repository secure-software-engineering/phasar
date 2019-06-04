#include <stdio.h>
#include <stdlib.h>

FILE *bar() {
	FILE *fp;
	return fp;
}

FILE *foo() {
  FILE *p = bar();
  p = fopen("bar.txt", "w+");
  return p;
}

int main() {
  FILE *f;
  f = foo();
  // fprintf(f, "%s %d", "Life is ", 42);
  fclose(f);

  return 0;
}
