extern int source();
extern void sink(int data);

struct S {
  int data;
  S(int data) : data(data) {}
};

int main() {
  int a = source();
  sink(a);
  S *s = new S(0);
  int b = a;
  sink(b);
  return 0;
}
