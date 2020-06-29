// name-mangled configuration options represented as external booleans
extern int _CfIf3K_CONFIG_A;
// the "definedness" of a configuration option is modeled as a separate boolean
extern int _Djkifd_CONFIG_A_defined;

int main() {
  int x = 0;
  if (_Djkifd_CONFIG_A_defined) {
    for (int i = 0; i < 10; ++i) {
      ++x;
    }
  }
  return x;
}
