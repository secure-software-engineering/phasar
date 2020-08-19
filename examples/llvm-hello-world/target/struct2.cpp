struct S {
  int x;
  int y;
};

int main(int argc, char **argv) {
  S s;
  if (argc - 1) {
    s.x = 4;
    s.y = 5;
  } else {
    s.x = 40;
    s.y = 50;
  }
  int z = s.x + s.y;
  return z;
}
