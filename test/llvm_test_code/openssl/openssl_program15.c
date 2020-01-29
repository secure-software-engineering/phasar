#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/crypto.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
void derive(EVP_KDF_CTX *kctx, unsigned char *derived, size_t keylen){
    if (EVP_KDF_derive(kctx, derived, keylen) <= 0) {
        perror("EVP_KDF_derive");
    }
}
void cleanse(unsigned char *derived, size_t keylen){
    if(time(NULL)%2==0)
        OPENSSL_cleanse(derived,keylen);
}
int main(int argc, char* argv[])
{
    EVP_KDF *kdf;
    EVP_KDF_CTX *kctx = NULL;
    size_t keylen=32;
    unsigned char *derived;
    derived=malloc(32*sizeof(unsigned char));
    OSSL_PARAM params[5], *p = params;

    /* Find and allocate a context for the HKDF algorithm */
    if ((kdf = EVP_KDF_fetch(NULL, "hkdf", NULL)) == NULL) {
        perror("EVP_KDF_fetch");
    }
    kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);    /* The kctx keeps a reference so this is safe */
    if (kctx == NULL) {
        perror("EVP_KDF_CTX_new");
    }

    /* Build up the parameters for the derivation */
    *p++ = OSSL_PARAM_construct_utf8_string("digest", "sha256", (size_t)7);
    *p++ = OSSL_PARAM_construct_octet_string("salt", "salt", (size_t)4);
    *p++ = OSSL_PARAM_construct_octet_string("key", "secret", (size_t)6);
    *p++ = OSSL_PARAM_construct_octet_string("info", "label", (size_t)5);
    *p = OSSL_PARAM_construct_end();
    if (EVP_KDF_CTX_set_params(kctx, params) <= 0) {
        perror("EVP_KDF_CTX_set_params");
    }

    /* Do the derivation */
    derive(kctx, derived, keylen);

    /* Use the 32 bytes as a Key and IV */
    const unsigned char *key = derived+0;
    const unsigned char  *iv = derived+16;

    printf("Key: ");
    for (size_t i=0; i<16; ++i)
        printf("%02x ", key[i]);
    printf("\n");

    printf("IV:  ");
    for (size_t i=0; i<16; ++i)
        printf("%02x ", iv[i]);
    printf("\n");

//    OPENSSL_cleanse(derived, sizeof(derived));
//free(derived)
//    derived=NULL;
    cleanse(derived, keylen);
    EVP_KDF_CTX_free(kctx);

    printf("Key: ");
    for (size_t i=0; i<16; ++i)
        printf("%02x ", key[i]);
    printf("\n");

    printf("IV:  ");
    for (size_t i=0; i<16; ++i)
        printf("%02x ", iv[i]);
    printf("\n");

    return 0;
}

