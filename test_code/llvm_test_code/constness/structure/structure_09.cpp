/* immutable: x */
struct A {
	int f = 42;
  virtual void foo();
};

int main() {
	A x;
	return 0;
}