int main() {
  int x = 5;
  int y = x - 2;
  int z =
#ifdef A
      x + 2;
#else
      y * 2
#endif
  ;
  long
#ifdef B
      long
#endif
          u = z ^ 7;

  y = u;
#ifdef A
  u = ++x;
#endif;
  return y;
}