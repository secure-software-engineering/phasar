
void print([[clang::annotate("psr.sink")]] int) {
  /// TODO: clang::annotate does not work with function declarations (e.g.
  /// extern functions or forward references)
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  print(argc);
}
