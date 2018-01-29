/* mutable: gint */
int gint = 10;

int main() {
	int *p = &gint;
	*p = 20;
	return 0;
}