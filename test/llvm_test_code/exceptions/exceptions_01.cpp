
extern "C" int rand();

int foo() {
  int x = 42;
  if (rand()) {
    x += 4;
  } else {
    throw 2353782;
  }

  return x;
}

int main() {
  int a = 3;
  try {
    a = foo();
  } catch (...) {
    a = -1;
  }
  return a;
}