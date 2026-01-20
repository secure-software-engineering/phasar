
int *Back(int *Ptr);

int *Forth(int *Ptr) {
  if (*Ptr > 0) {
    *Ptr = *Ptr - 1;
    return Back(Ptr);
  }

  return Ptr;
}

int *Back(int *Ptr) {
  if (*Ptr > 0) {
    *Ptr = *Ptr - 1;
    return Forth(Ptr);
  }

  return Ptr;
}

// Mutual recursion test
int main() {
  int k = 2;
  int l = 2;

  int *xx1 = Back(&k);
  k = 2;
  int *xx2 = Back(&k);

  int *yy1 = Back(&l);
  l = 4;
  int *yy2 = Back(&l);

  return *xx1;
}
