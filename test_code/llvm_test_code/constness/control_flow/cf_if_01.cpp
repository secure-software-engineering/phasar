/* i | %1 (ID: 1) | mem2reg */
extern bool cond;
int main() {
	int *i = new int(7);
	if (cond) {
		*i = 20;
	}
	return 0;
}