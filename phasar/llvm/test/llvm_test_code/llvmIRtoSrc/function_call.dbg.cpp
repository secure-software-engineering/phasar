int mult(int a, int b) { return a * b; }

int global = 42;

int main() {
  int i = mult(2, 4);
  int j = i + global;
  return 0;
}
