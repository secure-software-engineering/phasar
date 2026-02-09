
void print([[clang::annotate("psr.sink")]] int) {}

struct iterator {
  int *it{};

  void next() { //
    ++it;
  }
};

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  int arr[10]{};
  arr[4] = argc;

  for (iterator it = {arr}, end = {arr + 10}; it.it != end.it; it.next()) {
    print(*it.it);
  }
}
