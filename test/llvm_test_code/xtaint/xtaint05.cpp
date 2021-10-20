
void print([[clang::annotate("psr.sink")]] int) {}
extern int rand(void);

void foo(int x) {
  int buf;
  int *p = &buf;
  *p = x;
  if (rand())
    *p = 0;
  else
    *p = 1;

  print(buf);
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  foo(argc);
}