
struct LinkedList {
  int Val;
  LinkedList *Next;
};

int main() {
  LinkedList Foo;
  Foo = {0, &Foo};

  LinkedList Bar;
  Bar = {42, &Foo};

  LinkedList Baz = Bar;
  LinkedList Boar = Baz;

  Boar.Next = &Bar;
  Baz.Next = &Boar;

  return Boar.Val;
}
