#define NULL (void *)0

int data64 = 247265347;
int data32 = 12367576;
int main() {
  int *data =
#ifdef X64
      &data64
#elif defined(X86)
      &data32
#else
      NULL
#endif
      ;
  int *p = &data[1];
  int *q = q - 1;
  *q++;
  int r = *q - 1;
}