
void setValueToFortyTwo(int *x) { *x = 42; }

int main() {
  int i = 7;
  int j = 13;
  setValueToFortyTwo(&i);
  setValueToFortyTwo(&j);
  return j;
}
