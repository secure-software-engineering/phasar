
typedef struct _LinkedListOne {
  int Val;
  struct _LinkedListOne *Next;
} LinkedListOne;

typedef struct _LinkedListTwo {
  int Val;
  struct _LinkedListOne *Next;
} LinkedListTwo;

int main() {
  LinkedListOne Foo;
  Foo = (LinkedListOne){0, &Foo};
  LinkedListTwo Bar;
  Bar = (LinkedListTwo){42, &Foo};

  LinkedListOne Baz = Foo;
  LinkedListTwo Boar = Bar;

  LinkedListOne *AliasOne = Foo.Next;
  LinkedListOne *AliasTwo = Bar.Next;
  LinkedListOne *AliasThree = Baz.Next;
  LinkedListOne *AliasFour = Boar.Next;

  return 0;
}
