/* immutable: a (not *a!),p,i */
int* foo() {
  int *a = new int(42);
  *a = 99;
	return a;
}

int main() {
  int *p = foo();
	int *i = p;
	return 0;
}