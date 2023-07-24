
// This class has 4 entries in the vtable
// a construtor, test, 2 destructors
// for the destructors, see here
// https://eli.thegreenplace.net/2015/c-deleting-destructors-and-virtual-operator-delete/
class Base {
public:
  virtual void test();
  virtual ~Base();
};

void Base::test() {}

Base::~Base() {}

void test() {
  Base *b = new Base();
  b->test();
}
