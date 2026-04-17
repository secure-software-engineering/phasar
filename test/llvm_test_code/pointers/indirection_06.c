
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

typedef struct _LinkedListFive {
  int Val;
  struct _LinkedListFour *Next;
} LinkedListFive;

int main() {
  LinkedListOne Foo;
  Foo = (LinkedListOne){0, &Foo};
  LinkedListTwo Bar;
  Bar = (LinkedListTwo){42, &Foo};
  LinkedListThree Baz;
  Baz = (LinkedListThree){42, &Bar};
  LinkedListFour Boar;
  Boar = (LinkedListFour){42, &Baz};
  LinkedListFive Far;
  Far = (LinkedListFive){42, &Boar};

  LinkedListOne One = Foo;
  LinkedListTwo Two = Bar;
  LinkedListThree Three = Baz;
  LinkedListFour Four = Boar;
  LinkedListFive Five = Far;

  LinkedListOne *AliasOne = One.Next;
  LinkedListOne *AliasTwo = Two.Next;
  LinkedListTwo *AliasThree = Three.Next;
  LinkedListThree *AliasFour = Four.Next;
  LinkedListFour *AliasFive = Five.Next;
  LinkedListOne *AliasSix = Foo.Next;
  LinkedListOne *AliasSeven = Bar.Next;
  LinkedListTwo *AliasEight = Baz.Next;
  LinkedListThree *AliasNine = Boar.Next;
  LinkedListFour *AliasTen = Far.Next;

  return 0;
}
