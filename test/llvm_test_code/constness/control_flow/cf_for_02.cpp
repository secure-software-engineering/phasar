/* c | %1 (ID: 1) | mem2reg */
extern bool cond;
int main() {
	int *c = new int(0);
	for (int i = 0; i < 2; ++i) {
		if (cond)
      *c = 20;
	}
	return 0;
}