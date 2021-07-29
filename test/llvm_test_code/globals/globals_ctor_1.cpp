#include <cstdio>

int foo() { return 42; }

int global_foo = foo();

int main() { printf("Foo: %d\n", global_foo); }