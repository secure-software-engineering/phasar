
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
  int l = 2;

  int *xx1 = Back(&k);
  int *xx2 = Back(&k);
  int *yy1 = Forth(&l);
  int *yy2 = Forth(&l);

  return *xx2;
}
