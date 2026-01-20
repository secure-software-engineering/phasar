
typedef struct _LinkedList {
  int Val;
  struct _LinkedList *Next;
} LinkedList;

int main() {
  LinkedList Foo;
  Foo = (LinkedList){0, &Foo};

  LinkedList Bar;
  Bar = (LinkedList){42, &Foo};

  LinkedList Baz = Bar;
  LinkedList Boar = Baz;

  Boar.Next = &Bar;
  Baz.Next = &Boar;

  return (int)Boar.Next;
}
