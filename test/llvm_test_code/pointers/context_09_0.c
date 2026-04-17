
int *selfRecursion(int *Ptr) {
  if (*Ptr <= 0) {
    return Ptr;
  }

  *Ptr = *Ptr - 1;
  return selfRecursion(Ptr);
}

int main() {
  int k = 4;
  int l = 4;

  int *x = selfRecursion(&k);
  int *y = selfRecursion(&l);

  return *y;
}
