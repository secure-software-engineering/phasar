int add(int a, int b) { return a + b; }

void inc(int &i) { ++i; }

int main(int argc, char **argv) {
  int a = 10;
  int b = add(a, 42);
  inc(b);
  int c = b;
  return 0;
}
