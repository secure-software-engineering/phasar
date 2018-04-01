/* - | - | mem2reg */
void foo(int* a) {
}

int main() {
	int i = 10;
	int j = 13;
  foo(&i);
  foo(&j);
	return 0;
}