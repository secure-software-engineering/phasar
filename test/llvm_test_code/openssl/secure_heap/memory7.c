#include <openssl/crypto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(const char *fmt, ...);

int main() {
  size_t SECURE_MIN_HEAP_SIZE = 1024;
  size_t SECURE_MAX_HEAP_SIZE = 1024 * 1024;
  // error handling has been partially omitted
  CRYPTO_secure_malloc_init(SECURE_MAX_HEAP_SIZE /*size*/,
                            SECURE_MIN_HEAP_SIZE /*minsize*/);
  int sec_heap_init = CRYPTO_secure_malloc_initialized();
  printf("secure heap initialized: %d\n", sec_heap_init);
  size_t BUFFER_SIZE = 32;
  char *buffer = OPENSSL_secure_zalloc(BUFFER_SIZE);
  printf("bytes allocated in secure heap: %zu\n", CRYPTO_secure_used());
  strcpy(buffer, "MY-SECRET-DATA");
  // clear and free buffer in secure heap
  OPENSSL_secure_clear_free(buffer, BUFFER_SIZE);
  CRYPTO_secure_malloc_done();
  return 0;
}
