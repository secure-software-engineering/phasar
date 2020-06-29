
#ifdef USE_INT
int compute(int i) { return ++i; }
#else
double compute(double d) { return d * 2; }
#endif

int main() {
#ifdef USE_INT
  int x = compute(1);
#else
  double x = compute(1.11);
#endif
  return 0;
}
