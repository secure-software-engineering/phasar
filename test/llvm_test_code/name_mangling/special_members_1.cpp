#include <iostream>
using namespace std;

class MyClass {
public:
  MyClass() { cout << "ctor\n"; }
  ~MyClass() { cout << "dtor\n"; }
  MyClass(const MyClass &m) { cout << "copy\n"; }
  MyClass operator=(const MyClass &m) {
    cout << "cpy assign\n";
    return MyClass();
  }
  MyClass(MyClass &&m) { cout << "move\n"; }
  MyClass &operator=(MyClass &&m) {
    cout << "mv assign\n";
    return m;
  }
};

int main() {
  MyClass c;
  MyClass d(c);
  MyClass e, f;
  e = c;
  MyClass g(move(e));
  f = move(d);
  return 0;
}
