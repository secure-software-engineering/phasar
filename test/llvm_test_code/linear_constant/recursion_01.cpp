extern bool cond;
int decrement(int i) {
  if (cond) {
    return decrement(--i);
  }
  return -1;
}

int main() {
  int j = decrement(-42);
  return 0;
}
