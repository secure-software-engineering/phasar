extern int source();
extern void sink(int data);

struct S {
  int data;
  S(int data) : data(data) {}
};

int main() {
  int data = source();
  try {
    S *s = new S(0);
    try {
      data = source();
    } catch (...) {
    }
  } catch (...) {
  }
  sink(data);
  return 0;
}
