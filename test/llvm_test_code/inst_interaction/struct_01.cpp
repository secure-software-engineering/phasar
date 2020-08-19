
struct X {
  int i;
  int j;
  X(int i, int j) : i(i), j(j) {}
};

int main() {
  X x(10, 20);
  int a = x.i;
  return a;
}
