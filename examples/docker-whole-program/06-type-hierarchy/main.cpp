struct Animal {
  virtual ~Animal() = default;
  virtual void speak() const = 0;
};
struct Dog : Animal {
  void speak() const override;
};
struct Cat : Animal {
  void speak() const override;
};
struct Puppy : Dog {
  void speak() const override;
};
void Dog::speak() const {}
void Cat::speak() const {}
void Puppy::speak() const {}
int main() { return 0; }
