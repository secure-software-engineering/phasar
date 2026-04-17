
typedef struct _LinkedList {
  int Val;
  struct _LinkedList *Next;
} LinkedList;

int main() {
  LinkedList Foo;
  Foo = (LinkedList){0, &Foo};

  LinkedList *Alias = Foo.Next;

  return 0;
}
