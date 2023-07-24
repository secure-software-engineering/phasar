// handle basic global function pointer

int foo() { return 42; }

int bar() { return 13; }

int (*fptr)() = &bar;

int main() { int result = fptr(); }
