int TestInt = 6;
int TestInt2 = 12;

int *call4() { return &TestInt2; }
int *call3() { return &TestInt; }
int call2() { return *call3() + *call4(); }
int call1() { return call2(); }

int main(int /*argc*/, char * /*argv*/[]) {
  int CallReturn = call1();

  return 0;
}
