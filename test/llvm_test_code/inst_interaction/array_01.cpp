int main() {
  int i = 13;
  int j = 42;
  int buffer[16] = {0};
  buffer[0] = i;
  buffer[10] = j;
  int x = buffer[0];
  int y = buffer[10];
  int z = buffer[1];
  return 0;
}
