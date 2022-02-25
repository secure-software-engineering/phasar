bool rand();

int main() {
  int i;
  if (rand()) {
    i = 42;
  } else {
    i = 666;
  }
  return i;
}
