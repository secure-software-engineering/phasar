
struct X {
  int a, b;
};

struct Y : X {
  int c;
};

int global = 1;

int main() {
  int i;
  int j;
  int k = i + j;
  int *a = &i;
  int *b = &j;
  *b = 42;
  int *memory = new int(13);
  *memory += i + j;
  *memory += k;
  delete memory;
  Y *y = new Y;
  delete y;
  return 0;
}
