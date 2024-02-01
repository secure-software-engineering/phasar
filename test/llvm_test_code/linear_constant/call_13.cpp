
extern "C" int puts(int);

void use(int &p) { puts(p); }

int main() {
  int x = 42;
  use(x);
  int y = 43;
  y = x;
  y++;
  return y;
}
