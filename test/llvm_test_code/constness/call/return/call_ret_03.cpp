/* a | %1 (ID: 0) | mem2reg */
int* foo() {
	int *a = new int(42);
  return a;
}

int main() {
	// moved to register due to mem2reg
  int *p = foo();
	*p = 13;
	return 0;
}