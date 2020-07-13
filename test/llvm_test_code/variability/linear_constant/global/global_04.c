
typedef unsigned
#ifdef Bit64
    long long
#else
    int
#endif
        size_t;

size_t maximum;

typedef unsigned long long uint64_t;

#ifdef Bit64

uint64_t value = 2;
#else
int value_1 = 0;
int value_2 = 2;
#endif;

int main() {
  uint64_t local_value =
#ifdef Bit64
      value;
#else
      ((uint64_t)value_1 << 32) | value_2;
#endif
  uint64_t mx = local_value + 2;
  maximum = mx;

  return maximum - 1;
}