
void inputRefParam(int &ir) { // NOLINT
  ir = 42;
}

int main() {
  int VarIR;
  inputRefParam(VarIR);
  return VarIR;
}
