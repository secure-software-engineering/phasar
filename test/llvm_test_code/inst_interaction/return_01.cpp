
int returnIntegerLiteral() { return 9001; }

int main() {
  int localVar;
  localVar = 42;
  localVar = returnIntegerLiteral();
  return localVar;
}
