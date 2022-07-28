/* i | %1 (ID: 4) | mem2reg */
void foo(int *p) { *p = 42; }

void bar(int *b) { foo(b); }
int main() {
  int *i = new int(24);
  bar(i);
  return 0;
}
