/* mutable: i,k */
extern bool cond;

int main() {
	int i = 7;
	int k = 12;
	if (cond) {
		i = 20;
	} else {
		k = 1;
	}
	return 0;
}