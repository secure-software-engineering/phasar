
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

  LinkedList *AliasOne = Foo.Next;
  LinkedList *AliasTwo = Bar.Next;
  LinkedList *AliasThree = Baz.Next;
  LinkedList *AliasFour = Boar.Next;

  return 0;
}
