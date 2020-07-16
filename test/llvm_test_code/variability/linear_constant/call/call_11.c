
int foo(int);
int bar(int);

int foo(int x) {
#ifdef A
  if (x)
    return x + foo(x - 1);
  return x;
#else
  if (x > 0)
    return x + bar(x);
  return x;
#endif
}
int bar(int x) {
#ifdef A
  if (x)
    return x + foo(x - 1);
  return x;
#else
  return x + foo(x - 2);
#endif
}

int main() {
  int i = 3;
  int j = foo(i);
  return j - 1;
}