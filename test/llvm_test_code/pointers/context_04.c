
struct Lorem {
  int i;
  int *j;
};

int main() {
  struct Lorem Ipsum;

  int Value = 7;
  Ipsum.i = Value;
  Ipsum.j = &Value;

  return *Ipsum.j;
}
