/* a | %1 (ID: 0) | mem2reg */
int* foo() {
  int *a = new int(42);
  *a += 99;
	return a;
}

int main() {
  // stored in virtual register due to mem2reg
  int *p = foo();
	return 0;
}