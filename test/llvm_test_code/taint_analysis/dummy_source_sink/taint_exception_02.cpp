extern int source();
extern void sink(int data);

struct S {
  int data;
  S(int data) : data(data) {}
};

int main(int argc, char **argv) {
  S *s = new S(0);
  sink(argc);
  return 0;
}
