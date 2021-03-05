
extern void puts(const char *);
extern int rand();
int main() {
  const char *str1 = "Hello, World";
  const char *str2 = "Hello Hello";
  if (rand())
    str1 = str2;

  puts(str1);
  return 0;
}