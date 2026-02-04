
int Func() { return 1; }

int main() {
  int (*FuncPtr)();

  FuncPtr = &Func;

  return FuncPtr();
}
