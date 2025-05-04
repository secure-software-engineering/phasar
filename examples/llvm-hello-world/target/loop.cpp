int main(int argc, char **argv) {
  int sum = 0;
  for (int i = 1; i <= argc; ++i) {
    sum += i;
  }
  return sum;
}
