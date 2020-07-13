int main() {
  int i = 3;
  int j =
#ifdef A
      i + 3;
#elif defined B
      7
#else
#ifdef A
      4
#elif !defined C
      100 - i
#else
      -1
#endif
#endif
  ;

  j++;
  int k = 2 * j;
  return -k;
}