/* mutable: gint, i */
int *gint = nullptr;

void foo() {
	*gint += 13;
}

int main() {
	int i = 42;
	gint = &i;
	foo();
	return 0;
}