/* immutable: - */
int *gint;

void foo() {
  *gint = 99;
}

int main() {
  int i = 42;
  gint = &i;
  foo();
  return 0;
}
