
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

  int *x = Back(&k);

  k = 2;

  int *y = Back(&k);

  return *x;
}
