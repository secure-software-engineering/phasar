struct S {
  int a;
  int b;
  int c;
};

int main() {
  int i = 1;
  int j = 2;
  int k = 3;
  S l;
  l.a = i;
  l.b = j;
  l.c = k;
  int x = l.a;
  int y = l.b;
  int z = l.c;
  return 0;
}
