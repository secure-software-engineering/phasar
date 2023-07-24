int A = 42;
int B;

void initB() { B = 21; }

int main() {
  int C = B;

  initB();

  return A + B + C;
}
