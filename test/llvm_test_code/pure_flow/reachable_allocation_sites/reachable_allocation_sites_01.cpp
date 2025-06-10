int TestInt = 6;

int *call2() { return &TestInt; }
int *call1() { return call2(); }

int main(int /*argc*/, char * /*argv*/[]) {
  int *CallReturn = call1();

  return 0;
}
