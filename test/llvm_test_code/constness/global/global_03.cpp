/* g | @g (ID: 0) | mem2reg */
// llvm uses a special constructor for initializing global
// heap memory which results into an extra store operation
// to g; first one being the standard global variable instanziation
// with null.
int *g = new int(17);

void foo(int *p) {
  *p = 42;
}

int main() {
	// moved to register
  int *i = g;
  foo(i);
	return 0;
}