/* mutable: - */
void foo(int* a) {
	int b = *a;
}

int main() {
	int i = 10;
	int j = 13;
  foo(&i);
  foo(&j);
	return 0;
}