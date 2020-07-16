
/*****************************************************
 * Copied from setjmp.h
 * **************************************************/

#ifndef __THROWNL
#define __THROWNL __attribute__((__nothrow__))
#endif

#if __WORDSIZE == 64
typedef long int __jmp_buf[8];
#elif defined __x86_64__
__extension__ typedef long long int __jmp_buf[8];
#else
typedef int __jmp_buf[6];
#endif

#define _SIGSET_NWORDS (1024 / (8 * sizeof(unsigned long int)))
typedef struct {
  unsigned long int __val[_SIGSET_NWORDS];
} __sigset_t;

struct __jmp_buf_tag {
  /* NOTE: The machine-dependent definitions of `__sigsetjmp'
     assume that a `jmp_buf' begins with a `__jmp_buf' and that
     `__mask_was_saved' follows it.  Do not move these members
     or add others before it.  */
  __jmp_buf __jmpbuf;      /* Calling environment.  */
  int __mask_was_saved;    /* Saved the signal mask?  */
  __sigset_t __saved_mask; /* Saved signal mask.  */
};

typedef struct __jmp_buf_tag jmp_buf[1];

extern int setjmp(jmp_buf __env) __THROWNL;

extern void longjmp(struct __jmp_buf_tag __env[1], int __val) __THROWNL
    __attribute__((__noreturn__));

/*****************************************************
 * Actual test code
 * **************************************************/

jmp_buf cc;

void throwException(int exc) {
  if (exc)
    longjmp(cc, exc);
}

// similar to "https://en.cppreference.com/w/cpp/utility/program/setjmp"
int foo() {
  volatile int e = 0;
#ifdef A
  if (setjmp(cc) != 5) {
    throwException(++e);
  }
#else
  switch (setjmp(cc)) {
  case 0:
    throwException(5);
    break;
  case 5:
    return 5;
  default:
    // unreachable
    return 9973;
  }
#endif
  int j = e - 1;
  return j;
}

int main() {
  int j = foo();

  // A => 36; ~A => 45
  return j * 9;
}