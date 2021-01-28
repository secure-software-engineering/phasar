extern int source();
extern void sink(int data);

struct S {
  int data;
  S(int data) : data(data) {}
};

int main() {
  int data;
  try {
    S *s = new S(0);
    data = source();
  } catch (...) {
  }
  sink(data);
  return 0;
}
