
void print([[clang::annotate("psr.sink")]] int) {}

void bar(int *arr) {
  print(arr[0]);
  print(arr[1]);
}

void foo(int x) {
  int array[2];
  array[1] = x;
  bar(array);
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  foo(argc);
}
