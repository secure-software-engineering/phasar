bool rand();

int main() {
  int i;
  int j;
  int k = 9001;
  int *p;
  if (rand()) {
    p = &i;
  } else {
    p = &j;
  }
  *p = k;
  return *p;
}
