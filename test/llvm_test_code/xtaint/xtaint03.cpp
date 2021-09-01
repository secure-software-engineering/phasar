
void print([[clang::annotate("psr.sink")]] int) {}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {

  int array[2];
  array[1] = argc;

  print(array[0]);
  print(array[1]);
}