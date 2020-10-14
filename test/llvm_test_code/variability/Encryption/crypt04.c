// #include <openssl/evp.h>

#define NULL ((void *)0)

struct evp_cipher_ctx_st;
struct evp_cipher_st;

typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_cipher_st EVP_CIPHER;
typedef __SIZE_TYPE__ size_t;

extern EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void);
extern EVP_CIPHER *EVP_aes_256_cbc(void);
extern void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *);
int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, void *impl,
                       const unsigned char *key, const unsigned char *iv);
extern int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                             const unsigned char *in, int inl);
extern int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out,
                               int *outl);

extern int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                              void *impl, const unsigned char *key,
                              const unsigned char *iv);
extern int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                             const unsigned char *in, int inl);
extern int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm,
                               int *outl);

extern int EVP_CipherInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                             void *impl, const unsigned char *key,
                             const unsigned char *iv, int enc);
extern int EVP_CipherUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                            const unsigned char *in, int inl);
extern int EVP_CipherFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm,
                              int *outl);

extern size_t strlen(const char *);

int update(EVP_CIPHER_CTX *ctx, unsigned char *ciphertext,
           const char *plaintext, size_t plainlen) {
  return
#ifdef A
      // No error here
      EVP_EncryptUpdate(ctx, ciphertext, NULL, plaintext, plainlen);
#else
      // No error here
      EVP_DecryptUpdate(ctx, ciphertext, NULL, plaintext, plainlen);
#endif
}

int main(int argc, char *argv[]) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[] = "My Super secret encryption key!!";
  unsigned char iv[] = "Totally random IV";
#ifdef A
  EVP_EncryptInit_ex
#else
  // In this configuration we will have errors
  EVP_DecryptInit_ex
#endif
      (ctx, EVP_aes_256_cbc(), NULL, key, iv);

  const size_t arglen = strlen(argv[1]);
  unsigned char ciphertext[(arglen + 16) & ~16];

  update(ctx, ciphertext, NULL, argv[1], arglen);

  EVP_EncryptFinal_ex(ctx, ciphertext + 16, NULL); // First error here
  EVP_CIPHER_CTX_free(ctx);
  return 0;
}