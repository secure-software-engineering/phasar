/* mutable: i, gint */
int gint = 10;

int main() {
	int i = 7;
	i = gint;
	gint = 42;
	gint += 13;
	gint -= 17;
	return 0;
}