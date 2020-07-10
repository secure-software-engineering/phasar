int generate() {
#ifdef A
  return 42;
#else
  return
#ifdef B
      24
#else
      443
#endif
      ;
#endif
}

#ifdef EXPAND
long long
#else
int
#endif
foo() {
  return generate();
}

int main() {
  int result =
#ifdef EXPAND
      (int)(foo() * 334);
#else
      foo();
#endif
#ifndef EXPAND
  result *= 334;
#endif
  return result;
}