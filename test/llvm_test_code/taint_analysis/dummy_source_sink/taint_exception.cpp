extern int source();
extern void sink(int data);

struct S {
    int data;
    S(int data) : data(data) {}
};

void f() {
    S *s = new S(source());
    sink(s->data);
}

int main() {
  f();
  return 0;
}
