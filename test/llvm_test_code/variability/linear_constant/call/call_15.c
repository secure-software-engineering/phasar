typedef struct _named_params {
  int a;
  long long b;
  unsigned int c;
} named_params;

#define CALL(f, ...) f(&(named_params){__VA_ARGS__})

int compute(named_params *args) {
  return args->
#ifdef A
         b
#else
         a
#endif
         & 0xFFFF;
}

int main() {
  int i = 34;
  int j = CALL(compute, .b = 7, .a = 55, .c = 2534678);
  return j + 1;
}