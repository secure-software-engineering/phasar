int bar(int x) { return x * 2; }

int foo(int x) {
  switch (x) {
  case 0:
    do {
    case 1:
      x = bar(x);
    case 2:
      x = bar(x);
    case 3:
      x = bar(x);
#ifdef WIDE
    case 4:
      x = bar(x - 1);
    case 5:
      x = bar(x - 2);
#endif
    } while (x < 5);
  }
  return x;
}

int main() {
  int i = 3;
  int j = foo(i);
  return j;
}