extern int source();
extern void sink(int data);

struct S {
  int data;
  S(int data) : data(data) {}
};

int main() {
  int data = source();
  S *s = new S(0);
  sink(data);
  return 0;
}
