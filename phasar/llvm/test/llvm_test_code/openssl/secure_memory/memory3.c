#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>

void error(const char *fmt, ...);

int main() {
  size_t BUFFER_SIZE = 32;
  char *buffer = OPENSSL_zalloc(BUFFER_SIZE);
  char data[BUFFER_SIZE];
  strcpy(data, "MY-COOL-DATA");
  for (size_t i = 0; i < BUFFER_SIZE; ++i) {
    buffer[i] = data[i];
  }
  buffer[BUFFER_SIZE - 1] = '\0';
  printf("buffer: %s", buffer);
  printf("data: %s", buffer);
  // buffer freed but not cleared
  OPENSSL_free(buffer);
  return 0;
}
