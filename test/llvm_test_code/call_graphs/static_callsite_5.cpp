// Handle function call to static function of a class

using namespace std;

class Foo {
public:
  static int getNumFoos() { return numFoos; }

private:
  static int numFoos;
};

int Foo::numFoos = 0;

int main() { Foo::getNumFoos(); }
