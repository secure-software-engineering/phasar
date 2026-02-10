
int ret0() { return 0; }

int ret1() { return 1; }

int (*callback(int (*Func)()))() { return Func; }

int main() {
  int (*FuncPtrZero)() = &ret0;
  int (*FuncPtrOne)() = &ret1;

  FuncPtrZero = callback(&ret1);
  FuncPtrOne = callback(&ret0);

  return 0;
}
