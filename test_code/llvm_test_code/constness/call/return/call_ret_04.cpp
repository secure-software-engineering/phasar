/* immutable: i */
int* foo() {
	int a = 42;
	return &a;
}

int main() {
	int *i;
  i = foo();
  *i = 10;
	return 0;
}