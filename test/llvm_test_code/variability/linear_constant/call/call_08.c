
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

int exceptionId = 0;
jmp_buf cc;

void throwException(int exc) {
  exceptionId = exc;
  longjmp(cc, 1);
}

int foo() {
  if (setjmp(cc) == 0) {
    // try block
    throwException(42);
  } else {
    // catch block
    return 3;
  }
  return 4;
}

int main() {
  int i = 1;
  int j = foo();

  return i;
}