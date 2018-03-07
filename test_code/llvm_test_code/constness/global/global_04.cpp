/* immutable: - */
int gint = 10;

void foo() {
	gint = gint + 1;
}

int main() {
	foo();
	return 0;
}