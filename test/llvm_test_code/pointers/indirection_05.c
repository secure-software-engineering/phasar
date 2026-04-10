
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

typedef struct _LinkedListFour {
  int Val;
  struct _LinkedListThree *Next;
} LinkedListFour;

int main() {
  LinkedListOne Foo;
  Foo = (LinkedListOne){0, &Foo};
  LinkedListTwo Bar;
  Bar = (LinkedListTwo){42, &Foo};
  LinkedListThree Baz;
  Baz = (LinkedListThree){42, &Bar};
  LinkedListFour Boar;
  Boar = (LinkedListFour){42, &Baz};

  LinkedListOne One = Foo;
  LinkedListTwo Two = Bar;
  LinkedListThree Three = Baz;
  LinkedListFour Four = Boar;

  LinkedListOne *AliasOne = One.Next;
  LinkedListOne *AliasTwo = Two.Next;
  LinkedListTwo *AliasThree = Three.Next;
  LinkedListThree *AliasFour = Four.Next;
  LinkedListOne *AliasFive = Foo.Next;
  LinkedListOne *AliasSix = Bar.Next;
  LinkedListTwo *AliasSeven = Baz.Next;
  LinkedListThree *AliasEight = Boar.Next;

  return 0;
}
