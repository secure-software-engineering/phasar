/* mutable: i */
extern bool cond;

int main() {
	int i = 7;
	if (cond) {
		i = 20;
	}
	return 0;
}