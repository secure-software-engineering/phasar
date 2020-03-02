// name-mangled configuration options represented as external booleans
extern int _CfIf3K_CONFIG_A;
// the "definedness" of a configuration option is modeled as a separate boolean
extern int _Djkifd_CONFIG_A_defined;

unsigned factorial(unsigned n) {
  if (_Djkifd_CONFIG_A_defined) {
    if (n <= 1) {
      return 1;
    }
    return n * factorial(n - 1);
  } else {
    return 0;
  }
}

int main() {
  int x = 5;
  x = factorial(x);
  return x;
}
