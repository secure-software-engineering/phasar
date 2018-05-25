struct Adder {
  virtual int add(int, int) = 0;
};

struct R {
  int mult(int a, int b) { return a * b; }
};

struct S : Adder {
  virtual int add(int a, int b) override { return a + b; }
  int sub(int a, int b) { return a - b; }
};

struct T : Adder {
  virtual int add(int a, int b) override { return 2 * a + b; }
};

Adder *makeAdder() { return new S; }

int add(int a, int b) { return a + b; }

int (*F)(int, int) = &add;

Adder *A = makeAdder();

int (R::* M)(int, int) = &R::mult;

int main() {
  int a = add(4, 5);
  int x = A->add(1, 2);
  int y = F(1, 2);
  R r;
  int z = (r.*M)(2, 3);
  return 0;
}
