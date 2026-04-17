
int Func() { return 1; }

int main() {
  int (*FuncPtr)();
  int (**FuncPtr2)();

  FuncPtr = &Func;
  FuncPtr2 = &FuncPtr;

  return (*FuncPtr2)();
}
