int sum(int n, int arr[n]) {
  int result = 0;
  for (int i = 0; i < n; ++i) {
    result += arr[i];
  }
  return result;
}

int main() {
  int n = 42;
  int dyn_array[n];
  __builtin_memset(dyn_array, 0, n);
  int s =
#ifdef A
      sum(n, dyn_array)
#else
      1
#endif
      ;
  int sp1 = s + 1;
  // A => 3 (probably bot); ~A => 6
  return sp1 * 3;
}