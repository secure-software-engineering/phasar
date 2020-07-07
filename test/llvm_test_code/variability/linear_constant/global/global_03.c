int compress(long long x) { return x & -1; }

int (*compressor)(long long) =
#ifdef A
    0
#else
    &compress
#endif
    ;

int main() {
  int i = 42;
  long long j = i * 2147483650LL;
  int k =
#ifdef A
      compress(j)
#else
      -1
#endif;
}