
int *Back(int *Ptr);
int *Stop(int *Ptr);

int *Forth(int *Ptr) {
  if (*Ptr > 0) {
    *Ptr = *Ptr - 1;
    return Stop(Ptr);
  }

  return Ptr;
}

int *Back(int *Ptr) {
  if (*Ptr > 0) {
    *Ptr = *Ptr - 1;
    return Stop(Ptr);
  }

  return Ptr;
}

int *Stop(int *Ptr) {
  if (*Ptr % 2 == 1) {
    return Forth(Ptr);
  }

  return Back(Ptr);
}

// Mutual recursion test
int main() {
  int k = 2;
  int l = 3;

  int *x = Back(&k);
  int *y = Forth(&l);

  return *x;
}
