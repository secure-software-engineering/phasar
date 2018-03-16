/* immutable: i */
int gint = 10;

int main() {
	int *i = &gint;
  *i = 42;
	return 0;
}