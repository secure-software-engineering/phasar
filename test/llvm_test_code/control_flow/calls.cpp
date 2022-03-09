void var() {}
void tar() {}
void foo() {}
void bar() { var(); }
void other() {
  foo();
  bar();
  tar();
}
void yetanother() {
  other();
  tar();
}

int main() {
  var();
  other();
  yetanother();
  return 0;
}
