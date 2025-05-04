#include <iostream>
using namespace std;

int test() {
  cout << "test" << endl;
  return 5;
}

namespace testspace {

class foo {
public:
  foo bar(foo baz) { return baz; }
};
}; // namespace testspace

int main(int argc, char const *argv[]) {
  test();
  testspace::foo i;
  testspace::foo j = i.bar(i);
  return 0;
}
