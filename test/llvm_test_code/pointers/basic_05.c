
struct Lorem {
  int Ipsum;
};

int main() {
  struct Lorem i;
  struct Lorem *p = &i;
  p->Ipsum = 3;
}
