/* mutable: gint */
int gint = 10;

void foo() {
	gint = gint + 1;
}

int main() {
	int *i = &gint;
  foo();
  *i = 42;
	return 0;
}