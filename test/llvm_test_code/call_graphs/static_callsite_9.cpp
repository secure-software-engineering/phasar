// Handle function call of a nested class

class Outside {
public:
  class Inside {
  public:
    void f() {}
  };
};

int main() {
  Outside::Inside inside;
  inside.f();
}
