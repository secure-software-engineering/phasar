int id(int i) { return i; }

int main() {
  int i = 13;
  int j = i + 42;
  int k = id(j);
  return k;
}
