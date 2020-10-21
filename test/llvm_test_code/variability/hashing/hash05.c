// #include <openssl/evp.h>

#define NULL ((void *)0)

struct evp_md_ctx_st;
struct evp_md_st;

typedef struct evp_md_ctx_st EVP_MD_CTX;
typedef struct evp_md_st EVP_MD;
typedef __SIZE_TYPE__ size_t;

extern EVP_MD_CTX *EVP_MD_CTX_new(void);
extern EVP_MD *EVP_sha512_256(void);
extern int EVP_DigestInit_ex(EVP_MD_CTX *ctx, EVP_MD *md, void *impl);
extern int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *m, size_t mlen);
extern int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *digest,
                              unsigned int *dlen);
extern void EVP_MD_CTX_free(EVP_MD_CTX *ctx);

#ifdef A
void initialize(EVP_MD_CTX *ctx) {

  EVP_DigestInit_ex(ctx, EVP_sha512_256(), NULL);
}
#else
#define initialize(CTX) ((void)0)
#endif

int main() {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  initialize(ctx);

  const char msg[] = "Hello, World";
  EVP_DigestUpdate(ctx, msg, sizeof(msg));
  unsigned char digest[256];
  EVP_DigestFinal_ex(ctx, digest, NULL);
  EVP_MD_CTX_free(ctx);
  return 0;
}