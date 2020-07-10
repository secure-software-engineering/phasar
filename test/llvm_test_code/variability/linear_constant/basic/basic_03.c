int main() {
  int i = 3;
#ifdef A
  int j = 4;
#else
  int k = 5;
#endif

#ifndef A
  int j = 5;
#else
  int k = 4;
#endif

  j++;
  k -= 2;
}