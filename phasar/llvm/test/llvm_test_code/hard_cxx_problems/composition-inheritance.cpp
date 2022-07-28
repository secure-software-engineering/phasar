struct Base {
  int i;
  double d;
};

struct Stuff {
  char c;
  float f;
};

struct Child : Base {
  Stuff s;
};

int main() {
  Child c;
  return 0;
}
