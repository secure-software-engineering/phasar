char *foo(char *str) { return str; }

extern void puts(const char *);

int main() { puts(foo(0)); }
