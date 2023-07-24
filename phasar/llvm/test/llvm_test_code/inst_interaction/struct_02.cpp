
struct X {
  int i;
};

struct Y {
  X x;
  int j;
};

int main() {
  Y b;
  b.x.i = 1000;
  b.j = 2000;
  int k = b.x.i;
  int l = b.j;
  return 0;
}
