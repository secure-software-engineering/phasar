#include <stdio.h>
#include <stdlib.h>

void bar(FILE *p) {
	fclose(p);
}

void foo(FILE *p) {
  p = fopen("bar.txt", "w+");
}

int main() {
  FILE *f;
  foo(f);
  bar(f);

  return 0;
}
