/* immutable: - */
int *gint;

int main() {
	int i = 42;
	gint = &i;
	*gint = 99;
	return 0;
}