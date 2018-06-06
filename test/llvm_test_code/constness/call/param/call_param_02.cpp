/* i | %1 (ID: 4) | mem2reg */
void foo(int* a) {
  *a += 42;
}

int main() {
	int i = 10;
	foo(&i);
	return 0;
}