Please refer to:
    -include/openssl/crypto.h
    -crypto/mem.c
    -crypto/sec_mem.c

Compile with:
    $ clang -std=c11 -Wall -Wextra -I/home/philipp/GIT-Repos/openssl/include/ -L/home/philipp/GIT-Repos/openssl/ deletion1.c -o deletion1 -lcrypto -pthread -ldl

https://www.openssl.org/docs/man1.1.1/man3/OPENSSL_cleanse.html
---------------------------------------------------------------
The OPENSSL_* versions of malloc, free, etc. are macros that are replaced by calls to the respective CRYPTO_* functions. Those macros pass additional information such as file and line into the actual CRYPTO_* implementations before eventually performing the system calls. In essence, the OPENSSL versions provide additional protection and customization hooks for events such as allocation failure. Actual functionalities are implemented in crypto/mem.c.

https://www.openssl.org/docs/man1.1.0/man3/OPENSSL_secure_malloc.html
---------------------------------------------------------------------
In addition, OpenSSL provides a "secure heap" to help protect applications (particularly long-running servers) from pointer overruns or underruns that could return arbitrary data from the program's dynamic memory area, where keys and other sensitive information might be stored. The level and type of security guarantees depend on the operating system. It is a good idea to review the code and see if it addresses your threat model and concerns. If a secure heap is used, then private key BIGNUM values are stored there. This protects long-term storage of private keys, but will not necessarily put all intermediate values and computations there. Actual functionalities are implemented in crypto/mem_sec.c. If the secure heap is misconfigured the fallback OPENSSL_* versions in the above are used.
