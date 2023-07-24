__attribute__((constructor)) void before_main();
__attribute__((destructor)) void after_main();

void before_main() {}
void after_main() {}

struct S {
  int data;
  S(int data) : data(data) {}
  ~S() {}
};

S s(0);

int main() {}
