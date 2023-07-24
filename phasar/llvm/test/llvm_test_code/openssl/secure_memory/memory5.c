#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>

void error(const char *fmt, ...);

int main() {
  size_t BUFFER_SIZE = 256;
  unsigned char *buffer = OPENSSL_malloc(BUFFER_SIZE);
  for (size_t i = 0; i < BUFFER_SIZE; ++i) {
    buffer[i] = i;
  }
  // memset may have no effect (depending on usage)
  memset(buffer, 0, BUFFER_SIZE);
  OPENSSL_free(buffer);
  return 0;
}
