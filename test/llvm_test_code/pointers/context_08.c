
int *selfRecursion(int *Ptr) {
  if (*Ptr <= 0) {
    return Ptr;
  }
  return Ptr;
}

int main() {
  int k = 1;
  int *kptr = &k;

  int *x = selfRecursion(kptr);

  k = 1;

  int *y = selfRecursion(kptr);

  return *x;
}
