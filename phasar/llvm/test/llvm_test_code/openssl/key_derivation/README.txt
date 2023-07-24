Please refer to:
    -include/openssl/kdf.h
    -include/openssl/params.h
    -crypto/evp/kdf_meth.c
    -crypto/evp/kdf_lib.c
    -crypto/params.c

Compile with:
    $ clang -std=c11 -Wall -Wextra -I/home/philipp/GIT-Repos/openssl/include/ key-derivation1.c -o key-derivation1 /home/philipp/GIT-Repos/openssl/libcrypto.a -pthread -ldl

https://www.openssl.org/docs/manmaster/man3/EVP_KDF_fetch.html
--------------------------------------------------------------
The EVP KDF routines are a high level interface to Key Derivation Function algorithms and should be used instead of algorithm-specific functions. After creating a EVP_KDF_CTX for the required algorithm using EVP_KDF_CTX_new(), inputs to the algorithm are supplied using calls to EVP_KDF_CTX_set_params() before calling EVP_KDF_derive() to derive the key.
