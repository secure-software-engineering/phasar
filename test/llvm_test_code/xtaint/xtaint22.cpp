
void print([[clang::annotate("psr.sink")]] int) {}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  int arr[10]{};
  arr[4] = argc;

  for (int *it = arr, *end = arr + 10; it != end; ++it) {
    print(*it);
  }
}
