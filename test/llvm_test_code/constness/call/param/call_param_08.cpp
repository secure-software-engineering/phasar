/* i | %1 (ID: 3) | mem2reg */
void foo(int &a) {
  // a moved to virtual register
  a = 24;
  a = 2;
}

int main() {
  int i;
  foo(i);
  return 0;
}
