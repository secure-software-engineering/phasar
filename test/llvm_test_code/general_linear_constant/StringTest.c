
extern void puts(const char *);
int main() {
  const char *str1 = "Hello, World";
  const char *str2 = str1;
  puts(str2);
  return 0;
}