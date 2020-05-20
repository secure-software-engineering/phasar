#include <stdio.h>
#include <stdlib.h>

#include <openssl/crypto.h>

// name-mangled configuration options represented as external booleans
extern int _CfIf3K_CONFIG_A;
// the "definedness" of a configuration option is modeled as a separate boolean
extern int _Djkifd_CONFIG_A_defined;
extern int _Ckc8DF_CONFIG_B;
extern int _D8dDjJ_CONFIG_B_defined;

void error(const char *fmt, ...);

int main() {
  if(_D8dDjJ_CONFIG_B_defined){
	  size_t BUFFER_SIZE = 256;
	  unsigned char *buffer = OPENSSL_malloc(BUFFER_SIZE);
	  for (size_t i = 0; i < BUFFER_SIZE; ++i) {
		buffer[i] = i;
	  }

	  if (_Djkifd_CONFIG_A_defined) {
		OPENSSL_cleanse(buffer, BUFFER_SIZE);
	  }

	  // buffer freed but maybe not cleared
	  OPENSSL_free(buffer);
  }
  return 0;
}
