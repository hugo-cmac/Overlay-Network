#ifndef CRIPTOGRAPHY_H_
#define CRIPTOGRAPHY_H_

#include <cstddef>
#include <cstring>

#include <openssl/aes.h>
#include <openssl/ecdh.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

#define PKPCKTSIZE 92

namespace std{

    class Criptography{
        private:
            AES_KEY ekey;
            AES_KEY dkey;

            unsigned char* skey = NULL;
            
            EVP_PKEY* privateKey = NULL;
            EVP_PKEY* publicKey = NULL;


            EVP_PKEY* generateKey();

            EVP_PKEY* extractPublicKey(EVP_PKEY *privateKey);

        public:
            int publicKeySize = 0;
            size_t sharedKeySize = 0;

            Criptography();

            ~Criptography();

            //return pk pointer
            unsigned char* getPublicKey();

            int deriveShared(unsigned char *publicKey);

            unsigned char* getSharedKey();

            //pt +1 e len +1
            void encript(unsigned char* pt, int len);

            //pt +1 e len +1
            void decript(unsigned char* ct, int len);
        
    };

}

#endif