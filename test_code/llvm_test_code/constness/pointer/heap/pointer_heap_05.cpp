/* immutable: both new */
int main() {
  int *p = new int(42);
  delete p;
  p = new int(99);
  delete p;
	return 0;
}
