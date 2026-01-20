
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

  int *xx1 = selfRecursion(&k);
  k = 4;
  int *xx2 = selfRecursion(&k);
  int *yy1 = selfRecursion(&l);
  l = 4;
  int *yy2 = selfRecursion(&l);

  return *yy2;
}
