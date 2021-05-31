int a = 42;
int b;

void initB() { b = 21; }

int main() {
  int c = b;
  initB();
  return a + b + c;
}
