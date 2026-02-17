
int ret0() { return 0; }

int callback(int (*Func)()) { return Func(); }

int main() {
  int (*FuncPtr)() = &ret0;

  int Zero = callback(FuncPtr);

  return 0;
}
