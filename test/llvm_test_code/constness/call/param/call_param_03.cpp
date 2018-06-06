/* - | - | mem2reg */
void foo(int& a) {
	// removed due to mem2reg
  int b = a;
}

int main() {
	int i = 10;
	foo(i);
	return 0;
}