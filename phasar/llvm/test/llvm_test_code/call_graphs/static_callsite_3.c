// handle simple recursion

unsigned factorial(unsigned n) { return (n == 0) ? 1 : factorial(--n); }

int main(int argc, char **argv) { int f = factorial(argc); }
