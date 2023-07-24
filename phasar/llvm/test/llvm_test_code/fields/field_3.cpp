
struct X {
  int x;
  int y;
};

void sanitize(X &x) { x.x = 0; }

int main(int argc, char **argv) {
  X x;
  x.x = argc;
  x.y = 42;
}
