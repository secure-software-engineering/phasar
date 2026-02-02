
typedef struct _LinkedList {
  int Val;
  struct _LinkedList *Next;
} LinkedList;

int main() {
  LinkedList Foo;
  Foo = (LinkedList){0, &Foo};

  return (int)Foo.Next;
}
