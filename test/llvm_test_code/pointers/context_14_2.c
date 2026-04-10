
int ret0() { return 0; }

int ret1() { return 1; }

int ret2() { return 1; }

int (*callback(int (*Func)()))() { return Func; }

int main() {
  int (*FuncPtrZero)() = &ret0;
  int (*FuncPtrOne)() = &ret1;
  int (*FuncPtrTwo)() = &ret2;

  FuncPtrZero = callback(&ret2);
  FuncPtrOne = callback(&ret0);
  FuncPtrTwo = callback(&ret1);

  return 0;
}
