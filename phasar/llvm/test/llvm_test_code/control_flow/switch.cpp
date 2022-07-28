extern char c;

int main() {
  int i = 0;
  switch (c) {
  case 'A':
    i = 10;
    break;
  case 'B':
  case 'C':
    i = 20;
    break;
  case 'D':
    i = 30;
  default:
    i = -1;
  }
  return 0;
}
