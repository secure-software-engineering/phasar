/* immutable: i,p */
int gint = 10;

void foo(int *p) {
  *p = 42;
}

int main() {
	int *i = &gint;
  foo(i);
	return 0;
}