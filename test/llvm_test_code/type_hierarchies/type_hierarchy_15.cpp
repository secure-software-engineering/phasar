class Base {
  int x;
public:
  virtual void test();
  virtual ~Base();
};

class Child: public Base {
public:
  virtual ~Child();
};

// the definition of this method is missing
// void Base::test() {
// }

Base::~Base() {
}

Child::~Child() {
}

void use() {
  Child *c = new Child();

  delete c;
}


