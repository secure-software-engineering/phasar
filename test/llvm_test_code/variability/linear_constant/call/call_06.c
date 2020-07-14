// This would be for full-constant-propagation...
/*int compute(int x, int y
#ifdef A
            ,
            long long z
#endif
) {
  return x + y
#ifdef A
         + (int)z
#ifdef B
         + (int)(z >> 32)
#endif
#endif
      ;
}*/

int compute(int x
#ifdef A
            ,
            long long z
#endif
) {
#ifndef A
  return x + 7;
#else
  return (int)
#ifdef B
      (z >> 32)
#else
      z
#endif
          ;
#endif
}

int main() {
  int x = 5;
  int j = compute(x
#ifdef A
                  (long long) x |
                  (((long long)x + 3) << 32)
#endif
  );
  int k = j - 1;
  return k;
}