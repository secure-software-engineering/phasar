int main() {
  int buffer[3][3][3];
  buffer[1][1][1] = 42;
  int i = buffer[1][1][1];
  return i;
}
