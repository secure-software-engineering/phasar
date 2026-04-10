
typedef struct _LinkedListOne {
  int Val;
  struct _LinkedListOne *Next;
} LinkedListOne;

typedef struct _LinkedListTwo {
  int Val;
  struct _LinkedListOne *Next;
} LinkedListTwo;

typedef struct _LinkedListThree {
  int Val;
  struct _LinkedListTwo *Next;
} LinkedListThree;

int main() {
  LinkedListOne Foo;
  Foo = (LinkedListOne){0, &Foo};
  LinkedListTwo Bar;
  Bar = (LinkedListTwo){42, &Foo};
  LinkedListThree Baz;
  Baz = (LinkedListThree){42, &Bar};

  LinkedListOne One = Foo;
  LinkedListTwo Two = Bar;
  LinkedListThree Three = Baz;

  LinkedListOne *AliasOne = One.Next;
  LinkedListOne *AliasTwo = Two.Next;
  LinkedListTwo *AliasThree = Three.Next;
  LinkedListOne *AliasFour = Foo.Next;
  LinkedListOne *AliasFive = Bar.Next;
  LinkedListTwo *AliasSix = Baz.Next;

  return 0;
}
