// strcpy(dst, src) library summary:
//   param 0 (dst) -> ReturnValue  =>  ret aliases dst
//   param 1 (src) -> Parameter{0} =>  *dst = src
// The return value of strcpy must alias buf (arg 0).
#include <string.h>

int main(void) {
  char buf[64];
  char *p = strcpy(buf, "hello");
  (void)p;
  return 0;
}
