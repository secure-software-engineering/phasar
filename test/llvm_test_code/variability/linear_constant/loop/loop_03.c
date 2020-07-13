int main() {
  int start = 1;
  int end = 5;

  int sum = 0;
#ifdef UNROLL
  sum = start + (start + 1) + (start + 2) + (start + 3);
#else
  for (; start != end; ++start)
    sum += start;
#endif;
  int result = sum - 1;
  return result;
}