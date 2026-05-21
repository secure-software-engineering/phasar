
struct Y {
  int A = 99;
};

struct X {
  int A = 13;
  int B = 0;
};

int main() {
  X V;
  Y W;
  return V.A + V.B + W.A;
}
